use anyhow::{anyhow, Context};
use bitvec::prelude::*;
use png::ColorType;
use scan_fmt::scan_fmt;
use serde_json::Value;
use std::{
    collections::HashMap,
    fmt::Display,
    fs::File,
    io::{self, Write, Cursor},
    path::PathBuf,
    str::FromStr,
};
use structopt::StructOpt;
use byteorder::WriteBytesExt;
use byteorder::LittleEndian;

fn main() {
    match run() {
        Err(e) => eprintln!("{:?}", e),
        Ok(_) => {}
    }
}

fn color_to_port(color: &Color) -> u16 {
    let red = ((color.r as f64) / 32.0) as u16;
    let green = ((color.g as f64) / 32.0) as u16;
    let blue = ((color.b as f64) / 32.0) as u16;
    ((red & 0b100) >> 1)
        | ((red & 0b010) << 9)
        | ((red & 0b001) << 11)
        | ((green & 0b100) << 10)
        | ((green & 0b010) << 12)
        | ((green & 0b001) << 14)
        | ((blue & 0b100) << 5)
        | ((blue & 0b010) << 7)
        | ((blue & 0b001) << 9)
}

fn run() -> anyhow::Result<()> {
    let opt = Opt::from_args();
    let decoder = png::Decoder::new(File::open(opt.image)?);
    let mut reader = decoder.read_info()?;
    let mut buf = vec![0; reader.output_buffer_size()];
    let info = reader.info();
    let width = info.width;
    if info.color_type != ColorType::Indexed {
        Err(anyhow!("PNG is not indexed"))?;
    }
    let next_frame = reader.next_frame(&mut buf)?;
    let bytes = &buf[..next_frame.buffer_size()];
    let mut palette: HashMap<Color, u8> = HashMap::new();
    let mut palette_map: HashMap<Color, u8> = HashMap::new();
    let mut chunks = reader.info().palette.as_ref().unwrap().chunks_exact(3);
    let mut i: u8 = 0;
    while let Some(&[r, g, b]) = chunks.next() {
        palette.insert(Color { r, g, b }, i);
        i += 1;
    }
    if opt.color_map.is_some() {
        let color_map: Value = opt.color_map.unwrap();
        let map: HashMap<String, u8> = serde_json::from_value(color_map)?;
        for (k, v) in map {
            palette_map.insert(k.try_into()?, v);
        }
    }
    let mut original_bv = bitvec![u8, Msb0;];
    // PNG images will ignore the remaining bits of the byte if there
    // are no more pixels horizontally.
    let mut horizontal_pixels = 0;
    for i in 0..next_frame.buffer_size() {
        match next_frame.bit_depth {
            png::BitDepth::One => {
                for bit in bytes[i].view_bits::<Msb0>().iter() {
                    original_bv.push(false);
                    original_bv.push(false);
                    original_bv.push(*bit);
                    horizontal_pixels += 1;
                    if horizontal_pixels >= width {
                        horizontal_pixels = 0;
                        break;
                    }
                }
            }
            png::BitDepth::Two => {
                for bits in bytes[i].view_bits::<Msb0>().chunks_exact(2) {
                    original_bv.push(false);
                    original_bv.push(bits[0]);
                    original_bv.push(bits[1]);
                    horizontal_pixels += 1;
                    if horizontal_pixels >= width {
                        horizontal_pixels = 0;
                        break;
                    }
                }
            }
            png::BitDepth::Four => {
                for bits in bytes[i].view_bits::<Msb0>().chunks_exact(4) {
                    original_bv.push(bits[1]);
                    original_bv.push(bits[2]);
                    original_bv.push(bits[3]);
                    horizontal_pixels += 1;
                    if horizontal_pixels >= width {
                        horizontal_pixels = 0;
                        break;
                    }
                }
            }
            png::BitDepth::Eight | png::BitDepth::Sixteen => {
                Err(anyhow!("Invalid bit depth - Please make sure your indexed PNG does not use more than 8 colors."))?
            }
        }
    }

    let mut bv = bitvec![u8, Msb0;];

    // map the colors
    if !palette_map.is_empty() {
        for pixel in original_bv.chunks_exact(3) {
            let index = ((pixel[0] as u8) << 2) | ((pixel[1] as u8) << 1) | (pixel[2] as u8);
            let original_color = palette
                .iter()
                .find(|(_, v)| **v == index)
                .context(
                    "Image contains colors that it dosen't contain? Your PNG might be broken...",
                )?
                .0;
            let new_index = *palette_map.get(original_color).with_context(|| {
                anyhow!(
                    "The image contains a color ({}) that is not present in the mapping.",
                    original_color
                )
            })?;
            bv.extend(new_index.view_bits::<Lsb0>()[0..3].iter().rev());
        }
    } else {
        bv = original_bv.clone();
    }

    drop(original_bv);

    while (bv.len() / 3) % 8 != 0 {
        bv.push(false);
    }

    let mut out: Box<dyn Write> = if opt.output_file == "-" {
        Box::new(io::stdout())
    } else if let Ok(fd) = File::create(opt.output_file) {
        Box::new(fd)
    } else {
        Err(anyhow!("Invalid output file."))?
    };

    let mut effective_palette = &palette;
    if !palette_map.is_empty() {
        effective_palette = &palette_map;
    }

    let fallback_color = Color {r: 0, g: 0, b: 0};
    let mut sorted_colors: [&Color; 8] = [&fallback_color; 8];

    for (color, i) in effective_palette {
        sorted_colors[*i as usize] = color;
    }

    if opt.embed_color_palette {
	for color in sorted_colors {
	    out.write_u16::<LittleEndian>(color_to_port(color))?;
	}
    }

    if opt.include_color_palette || opt.output_type == OutputType::Code {
        for (i, color) in sorted_colors.iter().enumerate() {
            println!(
                "color_palette[{}] = COLOR(0b{:03b}, 0b{:03b}, 0b{:03b});",
                i,
                ((color.r as f64) / 32.0) as u8,
                ((color.g as f64) / 32.0) as u8,
                ((color.b as f64) / 32.0) as u8
            );
        }
    }
    if opt.output_type == OutputType::Code {
        write!(
            &mut out,
            "const uint8_t image[{}] = {{\n\t",
            bv.as_raw_slice().len()
        )?;
    }
    for bytes in bv.as_raw_slice().chunks_exact(3) {
        match opt.output_type {
            OutputType::CArray | OutputType::Code => {
                write!(
                    &mut out,
                    "0x{:02x}, 0x{:02x}, 0x{:02x}, ",
                    bytes[2], bytes[1], bytes[0]
                )?;
            }
            OutputType::HexNumber => {
                write!(&mut out, "{:02x}{:02x}{:02x}", bytes[2], bytes[1], bytes[0])?;
            }
            OutputType::Binary => {
                out.write(&[bytes[2], bytes[1], bytes[0]])?;
            }
        }
    }

    if opt.output_type == OutputType::Code {
        writeln!(
            &mut out,
            "\n}};\nfor(uint16_t i; i < {}; ++i)\n\tfront_buffer[i] = image[i];",
            bv.as_raw_slice().len()
        )?;
    }
    out.flush()?;
    Ok(())
}

#[derive(Debug, Hash, PartialEq, Eq)]
struct Color {
    r: u8,
    g: u8,
    b: u8,
}

impl Display for Color {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{:02x}{:02x}{:02x}", self.r, self.g, self.b)
    }
}

impl TryFrom<String> for Color {
    type Error = anyhow::Error;

    fn try_from(value: String) -> Result<Self, Self::Error> {
        let (r, g, b) = scan_fmt!(&value, "{2x}{2x}{2x}", [hex u8],[hex u8],[hex u8])?;
        Ok(Color { r, g, b })
    }
}

#[derive(Debug, PartialEq)]
enum OutputType {
    CArray,
    Code,
    HexNumber,
    Binary,
}

impl FromStr for OutputType {
    type Err = &'static str;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        Ok(match s {
            "binary" | "bin" => OutputType::Binary,
            "array" | "c" => OutputType::CArray,
            "hex" | "number" | "hexadecimal" => OutputType::HexNumber,
            "code" => OutputType::Code,
            _ => Err("Invalid Output type.")?,
        })
    }
}

#[derive(Debug, StructOpt)]
#[structopt(
    name = "image2mes",
    about = "Convert a indexed png image into a MES image."
)]
struct Opt {
    /// Input image to convert
    #[structopt(parse(from_os_str), short = "i", long = "image")]
    image: PathBuf,
    #[structopt(short = "c", long = "color-map")]
    /// Maps colors to a certian index.
    color_map: Option<Value>,
    /// Converion format
    #[structopt(short = "t", long = "output-type", default_value = "binary")]
    output_type: OutputType,
    /// Output file, can also be - for stdout
    #[structopt(short = "o", long = "output")]
    output_file: String,
    /// Include palette (c code)
    #[structopt(short = "p", long = "include-color-palette")]
    include_color_palette: bool,
    #[structopt(short = "e", long = "embed-color-palette")]
    embed_color_palette: bool,
}
