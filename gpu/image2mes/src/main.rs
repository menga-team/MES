use anyhow::anyhow;
use bitvec::prelude::*;
use png::{ColorType};
use std::{fs::File, io::{Write, self}, path::PathBuf, str::FromStr};
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
        bv.extend(&bytes[i].view_bits::<Msb0>()[1..=3]);
        bv.extend(&bytes[i].view_bits::<Msb0>()[5..=7]);
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
                "color_palette[{}] = get_port_config_for_color(0x{:02x});",
                i,
                ((((color.r as f64) / 32.0) as u8) << 5)
                    | ((((color.g as f64) / 32.0) as u8) << 2)
                    | (((color.b as f64) / 64.0) as u8)
            )?;
        }
    }

    for bytes in bv.as_raw_slice().chunks_exact(3) {
        match opt.output_type {
            OutputType::CArray => {
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
            OutputType::Code => {
                writeln!(
                    &mut out,
                    "buffer_a[i++] = 0x{:02x};\nbuffer_a[i++] = 0x{:02x};\nbuffer_a[i++] = 0x{:02x};",
                    bytes[2], bytes[1], bytes[0]
                )?;
            },
        }
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
