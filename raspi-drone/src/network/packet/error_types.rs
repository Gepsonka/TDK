use std::error::Error;
use std::fmt::{Display, Formatter};

#[derive(Debug)]
pub struct PacketPayloadSizeError {
    payload_size: u8,
}

impl PacketPayloadSizeError {
    fn new(payload_size: u8) -> Self {
        PacketPayloadSizeError{
            payload_size
        }
    }
}


impl Display for PacketPayloadSizeError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "Payload size must me smaller than {}! ({})", MAX_PAYLOAD_SIZE, self.payload_size)
    }
}

impl Error for PacketPayloadSizeError {}


#[derive(Debug)]
pub struct PacketSizeError {}

impl Display for PacketSizeError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "Packet size must me smaller than {}! ({})", MAX_PACKET_SIZE, self.payload_size)
    }
}

impl Error for PacketSizeError {}




#[derive(Debug)]
pub struct CRCError {}

impl Display for CRCError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "CRC mismatch")
    }
}

impl Error for CRCError {}

