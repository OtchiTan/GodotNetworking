mod message_type;
mod network_manager;

use crate::network_manager::NetworkManager;

fn main() -> std::io::Result<()> {
    let network_manager = NetworkManager::start("127.0.0.1:3630");

    match network_manager {
        Ok(network_manager) => loop {
            match network_manager.recv() {
                Ok(_) => {}
                Err(_e) => {
                    println!("{}", _e);
                }
            }
        },
        Err(_e) => {
            println!("{}", _e);
            Ok(())
        }
    }
}
