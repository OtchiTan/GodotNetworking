pub enum MessageType {
    Helo,
    Hsk,
    Ping,
    Data,
}

#[derive(Debug)]
pub struct EnumError;

impl TryFrom<u8> for MessageType {
    type Error = EnumError;

    fn try_from(value: u8) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(MessageType::Helo),
            1 => Ok(MessageType::Hsk),
            2 => Ok(MessageType::Ping),
            3 => Ok(MessageType::Data),
            _ => Err(EnumError),
        }
    }
}
