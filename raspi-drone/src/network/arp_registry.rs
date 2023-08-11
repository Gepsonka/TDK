use std::borrow::Borrow;
use std::collections::BTreeMap;
use std::hash::Hash;
use std::time::{Duration, SystemTime};
use aes_gcm::{AeadCore, Aes128Gcm, Key, KeySizeUser, Nonce};
use aes_gcm::aead::generic_array::ArrayLength;
use aes_gcm::aead::OsRng;
use crate::network::packet::{LoRaPacket, MAX_PACKET_SIZE, MAX_MESSAGE_SLICE_SIZE};
use crate::network::device_status::DeviceStatus;
use crate::network::device_type::DeviceType;





#[derive(Debug, Clone, Hash, Eq, PartialOrd)]
pub struct InitializationVector<NonceSize>
where NonceSize: ArrayLength<u8>
{
    iv: Nonce<NonceSize>,
    expiration_duration: Duration,
    creation_time: SystemTime
}


impl <NonceSize> InitializationVector<NonceSize>
where NonceSize: ArrayLength<u8>
{
    pub fn new(iv: Nonce<NonceSize>, expiration: Duration, creation_time: SystemTime) -> Self {
        let nonce = Aes128Gcm::generate_nonce(&mut OsRng);
        InitializationVector {
            iv,
            expiration_duration: expiration,
            creation_time,
        }
    }

    fn duration_since_creation(&self) -> Duration {
        Duration::from_millis(self.creation_time.elapsed().unwrap().as_millis() as u64)
    }
}

impl <NonceSize> PartialEq<Self> for InitializationVector<NonceSize>
where NonceSize: ArrayLength<u8>
{
    fn eq(&self, other: &Self) -> bool {
        self.iv == other.iv
    }
}

impl <NonceSize> Ord for InitializationVector<NonceSize>
where NonceSize: ArrayLength<u8> + Eq + PartialOrd
{
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        self.iv.as_slice().cmp(&other.iv.as_slice())
    }
}

#[derive(Debug, Clone, Hash)]
pub struct InitializationVectorContainer<NonceSize>
where NonceSize: ArrayLength<u8>
{
    initialization_vectors: BTreeMap<Nonce<NonceSize>, InitializationVector<NonceSize>>
}

impl <NonceSize> InitializationVectorContainer<NonceSize>
where NonceSize: ArrayLength<u8>
{
    pub fn new() -> Self {
        InitializationVectorContainer {
            initialization_vectors: BTreeMap::new(),
        }
    }
}



#[derive(Debug, Clone)]
pub struct ArpRegistry<PacketT, KeySize, NonceSize>
where KeySize: KeySizeUser,
NonceSize: ArrayLength<u8>,
{
    pub address: u8,
    pub device_type: Option<DeviceType>,
    pub device_status: DeviceStatus,
    pub secret_key: Option<Key<KeySize>>,
    pub packet_rx_vec: Vec<PacketT>,
    packet_tx_vec: Vec<PacketT>,
    pub rx_message: Vec<u8>,
    tx_message: Vec<u8>,
    faulty_packets: Vec<u8>,
    used_ivs: InitializationVectorContainer<NonceSize>,
    iv_expiration_duration: Option<Duration>
}


impl <KeySize, NonceSize> ArpRegistry<LoRaPacket, KeySize, NonceSize>
where KeySize: KeySizeUser,
NonceSize: ArrayLength<u8>,
{
    pub fn new(address: u8, device_status: DeviceStatus) -> Self {
        ArpRegistry {
            address,
            device_type: None,
            device_status,
            secret_key: None,
            packet_rx_vec: Vec::new(),
            packet_tx_vec: Vec::new(),
            rx_message: Vec::new(),
            tx_message: Vec::new(),
            faulty_packets: Vec::new(),
            used_ivs: InitializationVectorContainer::new(),
            iv_expiration_duration: Some(Duration::from_secs(5))
        }
    }

    pub fn build_packets_from_message<TagSize: ArrayLength<u8>>(&mut self, dest_address: u8) -> Result<(), ()> {
        if self.tx_message.len() == 0 {
            Err(())
        }

        self.packet_tx_vec.clear();
        if let Some(address) = self.address {
            for (i, chunk) in self.tx_message.chunks(MAX_MESSAGE_SLICE_SIZE).enumerate() {

            }
        } else {
            for (i, chunk) in self.tx_message.chunks(MAX_PACKET_SIZE).enumerate() {

            }
        }


        Ok(())
    }

    


}





