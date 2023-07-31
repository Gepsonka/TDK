use std::borrow::Borrow;
use std::collections::BTreeMap;
use std::hash::Hash;
use std::time::{Duration, SystemTime};
use aes_gcm::{AeadCore, Aes128Gcm, Key, KeySizeUser, Nonce};
use aes_gcm::aead::generic_array::ArrayLength;
use aes_gcm::aead::OsRng;
use crate::network::packet::LoRaPacket;
use std::marker::PhantomData;
use aes_gcm::aes::cipher::crypto_common::InnerUser;







#[derive(Debug, Clone, Copy, PartialEq)]
pub enum DeviceStatus {
    KeyExchangeInitiated,
}

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
    address: u8,
    pub(crate) device_status: DeviceStatus,
    pub(crate) secret_key: Option<Key<KeySize>>,
    pub(crate) packet_rx_vec: Vec<PacketT>,
    packet_tx_vec: Vec<PacketT>,
    pub(crate) rx_message: Vec<u8>,
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


}





