use crate::MessageType::Data;
use MessageType::Hsk;
use std::net::UdpSocket;

enum NetworkState {
    NotConnected,
    Connecting,
    Connected,
    Spurious,
}

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
            1 => Ok(Hsk),
            2 => Ok(MessageType::Ping),
            3 => Ok(MessageType::Data),
            _ => Err(EnumError), // Handle invalid values
        }
    }
}

fn main() -> std::io::Result<()> {
    loop {
        let socket = UdpSocket::bind("127.0.0.1:3630")?;

        // Receives a single datagram message on the socket. If `buf` is too small to hold
        // the message, it will be cut off.
        let mut buf = [0u8; 1500];
        let (amt, src) = socket.recv_from(&mut buf)?;

        // Redeclare `buf` as slice of the received data and send reverse data back to origin.
        let buf = &mut buf[..amt];
        buf.reverse();

        let parsed_message = MessageType::try_from(buf[0]);
        match parsed_message {
            Ok(v) => match v {
                MessageType::Helo => {
                    println!("Helo");
                    let mut hsk_buf = [0u8; 1500];
                    hsk_buf[0] = Hsk as u8;
                    socket.send_to(&hsk_buf, &src)?;
                }
                MessageType::Hsk => {
                    let mut hsk_buf = [0u8; 1500];
                    hsk_buf[0] = Data as u8;
                    socket.send_to(&hsk_buf, &src)?;
                }
                MessageType::Ping => {}
                MessageType::Data => {}
            },
            Err(err) => {
                println!("Error: {:?}", err);
            }
        }
    } // the socket is closed here
    Ok(())
}
