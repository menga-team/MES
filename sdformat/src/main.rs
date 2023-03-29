use anyhow::anyhow;
use clap::{arg, Command};
use serde::de;
use serde::Deserialize;
use serde_json::Value;
use std::env;
use std::fmt;
use std::fs;
use std::fs::File;
use std::io::Read;
use std::path::{Path, PathBuf};

const PROJECT_FILE: &str = "mesproj.json";

#[derive(Deserialize, Debug)]
struct Game {
    pub name: MESString,
    pub author: MESString,
    pub version: Version,
    pub icon: BinaryDataFile, // palette is in included with the icon.
    pub game: BinaryDataFile,
}

// The name is bad... but it's essentially a line which only fits 26
// characters.
struct MESString {
    pub chars: [u8; 26],
}

// Will read the file in the json document and read the bytes in the
// specified file.
#[derive(Debug)]
struct BinaryDataFile {
    bytes: Vec<u8>,
}

#[derive(Deserialize, Debug)]
struct Version {
    pub major: u8,
    pub minor: u8,
    pub patch: u8,
}

impl<'de> de::Deserialize<'de> for MESString {
    fn deserialize<D>(deserializer: D) -> Result<MESString, D::Error>
    where
        D: de::Deserializer<'de>,
    {
        struct MESStringVisitor;

        impl<'de> de::Visitor<'de> for MESStringVisitor {
            type Value = MESString;

            fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                formatter.write_str(
                    "String using only characters up to 0xff and a maximum of 26 characters.",
                )
            }

            fn visit_str<E>(self, value: &str) -> Result<Self::Value, E>
            where
                E: de::Error,
            {
                let mut mstr = MESString { chars: [0; 26] };
                let mut i: usize = 0;
                let chars_in_string = value.chars().count();
                if chars_in_string > 26 {
                    Err(de::Error::invalid_length(
                        chars_in_string,
                        &"String with maximum 26 characters",
                    ))?
                }
                for char in value.chars() {
                    if char as u32 > 0xff {
                        Err(de::Error::invalid_value(
                            de::Unexpected::Char(char),
                            &"Characters not larger than 0xff",
                        ))?
                    } else {
                        mstr.chars[i] = char as u8;
                        i += 1;
                    }
                }
                Ok(mstr)
            }
        }
        deserializer.deserialize_identifier(MESStringVisitor)
    }
}

impl<'de> de::Deserialize<'de> for BinaryDataFile {
    fn deserialize<D>(deserializer: D) -> Result<BinaryDataFile, D::Error>
    where
        D: de::Deserializer<'de>,
    {
        struct BinaryDataFileVisitor;

        impl<'de> de::Visitor<'de> for BinaryDataFileVisitor {
            type Value = BinaryDataFile;

            fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                formatter.write_str("Path to a readable File.")
            }

            fn visit_str<E>(self, value: &str) -> Result<Self::Value, E>
            where
                E: de::Error,
            {
                let mut file = File::open(value).map_err(|_| {
                    de::Error::invalid_value(
                        de::Unexpected::Str(value),
                        &"Path to a readable File.",
                    )
                })?;
                let metadata = fs::metadata(value).map_err(|_| {
                    de::Error::invalid_value(
                        de::Unexpected::Other("Failed to get Metadata from File."),
                        &"Path to a readable File.",
                    )
                })?;
                let mut buffer = vec![0; metadata.len() as usize];
                file.read(&mut buffer).map_err(|_| {
                    de::Error::invalid_value(
                        de::Unexpected::Other("Failed to read File."),
                        &"Path to a readable File.",
                    )
                })?;
                Ok(BinaryDataFile { bytes: buffer })
            }
        }
        deserializer.deserialize_identifier(BinaryDataFileVisitor)
    }
}

impl fmt::Display for MESString {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        for char in self.chars {
            write!(f, "{}", char::from_u32(char as u32).unwrap())?
        }
        Ok(())
    }
}

impl fmt::Debug for MESString {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "\"{}\"", self)
    }
}

fn main() -> anyhow::Result<()> {
    let cmd = clap::Command::new("sdformat")
	.subcommand_required(true)
	.subcommand(
	    Command::new("iso")
		.about("Will create a iso image to burn.")
		.arg(arg!(<PROJECT> "The directory of the project. (directory with mesproj.json file)")
		     .value_parser(clap::value_parser!(PathBuf))
		)
		.arg_required_else_help(true),)
	.subcommand(
	    Command::new("flash")
		.about("Flash a generated iso file")
		.arg(arg!(<ISO> "The iso file to flash into the SD-Card")
		     .value_parser(clap::value_parser!(PathBuf))
		)
		.arg(arg!(<DEVICE> "The device to write to")
		     .value_parser(clap::value_parser!(PathBuf))
		)
		.arg_required_else_help(true));
    let matches = cmd.get_matches();
    match matches.subcommand() {
        Some(("iso", sub_matches)) => {
            let dir: PathBuf = sub_matches.get_one::<PathBuf>("PROJECT").unwrap().clone();
            env::set_current_dir(&dir)?;
            let project_file = std::fs::File::open(PROJECT_FILE);
            if let Err(e) = project_file {
                Err(anyhow!(
                    "Invalid project directory - Could not open {}: {}",
                    PROJECT_FILE,
                    e,
                ))?
            } else {
                let mut project_file = project_file?;
                let mut contents = String::new();
                project_file.read_to_string(&mut contents)?;
                let project_file: Value = serde_json::from_str(&contents)?;
                let game: Game = serde_json::from_value(project_file["sd_information"].to_owned())?;
		drop(project_file);
		drop(contents);
                println!("{:#?}", game);
		
            }
        }
        Some(("flash", sub_matches)) => {}
        _ => unreachable!(),
    }
    Ok(())
}
