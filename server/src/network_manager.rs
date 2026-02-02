use crate::message_type::MessageType;
use std::io;
use std::io::ErrorKind;
use std::net::{SocketAddr, UdpSocket};
use std::str;

pub struct NetworkManager {
    socket: UdpSocket,
}

impl NetworkManager {
    pub fn start(addr: &str) -> io::Result<Self> {
        let socket = UdpSocket::bind(addr);
        println!("UDP server started on {}", addr);

        match socket {
            Ok(socket) => {
                socket.set_nonblocking(true)?;
                Ok(Self { socket })
            }
            Err(error) => Err(io::Error::new(io::ErrorKind::Other, error)),
        }
    }

    fn handle_helo(&self, addr: SocketAddr) {
        println!("Receive Helo");
        let mut helo_buf = [0u8; 1500];
        helo_buf[0] = MessageType::Helo as u8;
        self.socket
            .send_to(&helo_buf, &addr)
            .expect("Error Message sending");
    }

    fn handle_hsk(&self, addr: SocketAddr) {
        println!("Receive Ksk");
        let mut hsk_buf = [0u8; 1500];
        hsk_buf[0] = MessageType::Hsk as u8;
        self.socket
            .send_to(&hsk_buf, &addr)
            .expect("Error Message sending");
    }

    fn handle_ping(&self, addr: SocketAddr) {
        println!("Receive Ping");
        let mut ping_buf = [0u8; 1500];
        ping_buf[0] = MessageType::Ping as u8;
        self.socket
            .send_to(&ping_buf, &addr)
            .expect("Error Message sending");
    }

    fn handle_data(&self, addr: SocketAddr, buffer: &[u8]) {
        println!("Receive Data");
        match str::from_utf8(buffer) {
            Ok(s) => {
                println!("{}", s);
            }
            Err(e) => {
                eprintln!("Invalid UTF-8 sequence: {}", e);
            }
        }

        let mut response_data = [0u8; 1500];
        response_data[0] = MessageType::Data as u8;
        self.socket
            .send_to(&response_data, &addr)
            .expect("Error Message sending");
    }

    pub fn recv(&self) -> io::Result<()> {
        let mut buf = [0u8; 1500];

        match self.socket.recv_from(&mut buf) {
            Ok((received_bytes, sender_addr)) => {
                let buf = &mut buf[..received_bytes];

                let parsed_message = MessageType::try_from(buf[0]);
                match parsed_message {
                    Ok(v) => match v {
                        MessageType::Helo => Self::handle_helo(self, sender_addr),
                        MessageType::Hsk => Self::handle_hsk(self, sender_addr),
                        MessageType::Ping => Self::handle_ping(self, sender_addr),
                        MessageType::Data => Self::handle_data(self, sender_addr, buf),
                    },
                    Err(err) => {
                        println!("Error: {:?}", err);
                    }
                }
            }
            Err(ref e) if e.kind() == ErrorKind::WouldBlock => {}
            Err(e) => {
                eprintln!("An error occurred: {}", e);
            }
        }
        Ok(())
    }
}
