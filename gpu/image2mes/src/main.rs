use anyhow::anyhow;
use bitvec::prelude::*;
use png::ColorType;
use std::{fs::File, path::PathBuf, str::FromStr};
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
    let mut palette = Vec::new();
    let mut iter = reader.info().palette.as_ref().unwrap().iter();
    while let (Some(&r), Some(&g), Some(&b)) = (iter.next(), iter.next(), iter.next()) {
        palette.push(Color { r, g, b });
    }
    let mut bv = bitvec![u8, Msb0;];
    for i in 0..next_frame.buffer_size() {
        print!("gpu_set_pixel(buffer_a, {}, {});", i *2, (bytes[i]  & 0b01110000) >> 4);
        print!("gpu_set_pixel(buffer_a, {}, {});", i *2+1, bytes[i] & 0b00000111);

        bv.extend(&bytes[i].view_bits::<Msb0>()[5..8]);
        bv.extend(&bytes[i].view_bits::<Msb0>()[0..3]);
    }
    // dbg!(palette);
    let mut group: u8 = 0;
    let mut i = 0;
    for color in palette {
        println!(
            "color_palette[{}] = get_port_config_for_color({});",
            i,
            ((((color.r as f64) / 32.0) as u8) << 5)
                | ((((color.g as f64) / 32.0) as u8) << 2)
                | ((((color.b as f64) / 64.0) as u8))
        );
        i += 1;
    }
    i = 0;
    for hex in bv.as_raw_slice() {
        // if group == 0 {
        //     print!("buffer_a[{}] = 0x", i);
        // }
        // print!("{:02x}", hex);
        // group += 1;
        // if group == 4 {
        //     print!(";");
        //     group = 0;
        //     i += 1;
        // }
        i += 1;
    }
    Ok(())
}

#[derive(Debug)]
struct Color {
    r: u8,
    g: u8,
    b: u8,
}

#[derive(Debug)]
enum OutputType {
    CArray,
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
    /// Output file
    #[structopt(short = "o", long = "output")]
    output_file: PathBuf,
}
