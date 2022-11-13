use anyhow::anyhow;
use bitvec::prelude::*;
use png::ColorType;
use std::{
    fs::File,
    io::{self, Write},
    path::PathBuf,
    str::FromStr,
};
use structopt::StructOpt;

fn main() {
    match run() {
        Err(e) => eprintln!("{:?}", e),
        Ok(_) => {}
    }
}

fn run() -> anyhow::Result<()> {
    let opt = Opt::from_args();
    let decoder = png::Decoder::new(File::open(opt.image)?);
    let mut reader = decoder.read_info()?;
    let mut buf = vec![0; reader.output_buffer_size()];
    let info = reader.info();
    if info.color_type != ColorType::Indexed {
        Err(anyhow!("PNG is not indexed"))?;
    }
    let next_frame = reader.next_frame(&mut buf)?;
    let bytes = &buf[..next_frame.buffer_size()];
    let mut palette: Vec<Color> = Vec::new();
    let mut chunks = reader.info().palette.as_ref().unwrap().chunks_exact(3);
    while let Some(&[r, g, b]) = chunks.next() {
        palette.push(Color { r, g, b });
    }
    let mut bv = bitvec![u8, Msb0;];
    for i in 0..next_frame.buffer_size() {
        match next_frame.bit_depth {
            png::BitDepth::One => {
                for bit in bytes[i].view_bits::<Msb0>().iter() {
                    bv.push(false);
                    bv.push(false);
                    bv.push(*bit);
                }
            },
            png::BitDepth::Two => {
                for bits in bytes[i].view_bits::<Msb0>().chunks_exact(2) {
                    bv.push(false);
                    bv.push(bits[0]);
                    bv.push(bits[1]);
                }
            },
            png::BitDepth::Four => {
                bv.extend(&bytes[i].view_bits::<Msb0>()[1..=3]);
                bv.extend(&bytes[i].view_bits::<Msb0>()[5..=7]);        
            },
            png::BitDepth::Eight => todo!(),
            png::BitDepth::Sixteen => todo!(),
        }
    }

    let mut out: Box<dyn Write> = if opt.output_file == "-" {
        Box::new(io::stdout())
    } else if let Ok(fd) = File::create(opt.output_file) {
        Box::new(fd)
    } else {
        Err(anyhow!("Invalid output file."))?
    };

    if opt.include_color_palette || opt.output_type == OutputType::Code {
        for (i, color) in palette.iter().enumerate() {
            writeln!(
                &mut out,
                "color_palette[{}] = get_port_config_for_color(0b{:03b}, 0b{:03b}, 0b{:03b});",
                i,
                ((color.r as f64) / 32.0) as u8,
                ((color.g as f64) / 32.0) as u8,
                ((color.b as f64) / 32.0) as u8
            )?;
        }
    }
    if opt.output_type == OutputType::Code {
        write!(
            &mut out,
            "uint8_t __attribute__((section (\".rodata\"))) image[{}] = {{\n\t",
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

#[derive(Debug)]
struct Color {
    r: u8,
    g: u8,
    b: u8,
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
    /// Converion format
    #[structopt(short = "t", long = "output-type", default_value = "binary")]
    output_type: OutputType,
    /// Output file, can also be - for stdout
    #[structopt(short = "o", long = "output")]
    output_file: String,
    /// Include palette (c code)
    #[structopt(short = "p", long = "include-color-palette")]
    include_color_palette: bool,
}
