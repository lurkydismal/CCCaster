use std::{error::Error, fmt, io};

#[derive(Debug)]
pub enum AppError {
    Message(String),
    Unsupported(&'static str),
    Io(io::Error),
    Json(serde_json::Error),
}

impl fmt::Display for AppError {
    fn fmt(&self, l_f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Message(l_msg) => write!(l_f, "{l_msg}"),
            Self::Unsupported(l_msg) => write!(l_f, "{l_msg}"),
            Self::Io(l_err) => write!(l_f, "{l_err}"),
            Self::Json(l_err) => write!(l_f, "{l_err}"),
        }
    }
}

impl Error for AppError {}

impl From<String> for AppError {
    fn from(l_value: String) -> Self {
        Self::Message(l_value)
    }
}

impl From<&'static str> for AppError {
    fn from(l_value: &'static str) -> Self {
        Self::Message(l_value.to_string())
    }
}

impl From<io::Error> for AppError {
    fn from(l_err: io::Error) -> Self {
        Self::Io(l_err)
    }
}

impl From<serde_json::Error> for AppError {
    fn from(l_err: serde_json::Error) -> Self {
        Self::Json(l_err)
    }
}

pub type Result<T> = std::result::Result<T, AppError>;
