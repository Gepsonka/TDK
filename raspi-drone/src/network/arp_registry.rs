use std::collections::{BTreeMap, LinkedList};
use std::hash::Hash;
use std::time::{Duration, SystemTime};
use aes_gcm::aes::cipher::ArrayLength;
use aes_gcm::{AeadCore, Aes128Gcm, KeySizeUser, Nonce};
use aes_gcm::aead::OsRng;
use crate::network::packet::Packet;


pub trait InitializationVectorContainer<NonceSize>
    where NonceSize: ArrayLength<u8> + Clone + Copy + Eq + Hash
{
    fn delete_expired_ivs(&mut self);
    fn add_iv(&mut self, iv: InitializationVector<NonceSize>);
    fn check_if_iv_is_used(&self, iv: &Nonce<NonceSize>) -> bool;
}

pub trait ArpRegistry<SecretKey>
    where SecretKey: KeySizeUser{
    fn set_addr(&mut self, addr: u8);
    fn set_secret_key(&mut self, secret_key: SecretKey);
    fn set_device_status(&mut self, device_status: DeviceStatus);
    fn set_iv_expiration_duration(&mut self, duration: Duration);
}


#[derive(Debug, Clone, PartialEq)]
pub enum DeviceStatus {
    KeyExchangeInitiated,
}

#[derive(Debug, Clone, Hash)]
pub struct InitializationVector<NonceSize>
    where NonceSize: ArrayLength<u8> + Clone + Copy + Eq + Hash
{
    iv: Nonce<NonceSize>,
    expiration_duration: Duration,
    creation_time: SystemTime
}


impl <NonceSize> InitializationVector<NonceSize>
    where NonceSize: ArrayLength<u8> + Clone + Copy + Eq + Hash
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
    where NonceSize: ArrayLength<u8> + Clone + Copy + Eq + Hash
{
    fn eq(&self, other: &Self) -> bool {
        self.iv == other.iv
    }
}




#[derive(Debug)]
pub struct AESInitializationVectorContainer<NonceSize>
    where NonceSize: ArrayLength<u8> + Clone + Copy + Eq + Hash
{
    initialization_vectors: BTreeMap<Nonce<NonceSize>, InitializationVector<NonceSize>>
}

impl <NonceSize> AESInitializationVectorContainer<NonceSize>
    where NonceSize: ArrayLength<u8> + Clone + Copy + Eq + Hash
{
    pub fn new() -> Self {
        AESInitializationVectorContainer {
            initialization_vectors: BTreeMap::new(),
        }
    }
}

impl<NonceSize> InitializationVectorContainer<NonceSize> for AESInitializationVectorContainer<NonceSize>
    where NonceSize: ArrayLength<u8> + Clone + Copy + Eq + Hash,
{
    fn delete_expired_ivs(&mut self) {
        for (key, val) in self.initialization_vectors.clone().into_iter() {
            if val.creation_time.elapsed().unwrap() > val.expiration_duration {
                self.initialization_vectors.remove(&key);
            }
        }
    }

    fn add_iv(&mut self, iv: InitializationVector<NonceSize>) {
        self.initialization_vectors.insert(iv.iv, iv.clone());
    }

    fn check_if_iv_is_used(&self, iv: &Nonce<NonceSize>) -> bool{
        self.initialization_vectors.contains_key(iv)
    }
}

#[derive(Debug)]
pub struct LoRaArpRegistry<PacketT, SecretKey, NonceSize>
    where PacketT: Packet + Into<Vec<u8>> + From<Vec<u8>>,
          SecretKey: KeySizeUser,
          NonceSize: ArrayLength<u8> + Clone + Copy + Eq + Hash
{
    address: u8,
    device_status: DeviceStatus,
    secret_key: Option<SecretKey>,
    packet_rx_vec: LinkedList<PacketT>,
    packet_tx_vec: LinkedList<PacketT>,
    rx_message: Vec<u8>,
    tx_message: Vec<u8>,
    used_ivs: AESInitializationVectorContainer<NonceSize>,
    iv_expiration_duration: Option<Duration>
}


impl <PacketT, SecretKey, NonceSize> LoRaArpRegistry<PacketT, SecretKey, NonceSize>
    where PacketT: Packet + Into<Vec<u8>> + From<Vec<u8>>,
          SecretKey: KeySizeUser,
          NonceSize: ArrayLength<u8> + Clone + Copy + Eq + Hash
{
    pub fn new(address: u8, device_status: DeviceStatus) -> Self {
        LoRaArpRegistry {
            address,
            device_status,
            secret_key: None,
            packet_rx_vec: LinkedList::new(),
            packet_tx_vec: LinkedList::new(),
            rx_message: Vec::new(),
            tx_message: Vec::new(),
            used_ivs: AESInitializationVectorContainer::new(),
            iv_expiration_duration: None
        }
    }
}

impl <PacketT, SecretKey, NonceSize> ArpRegistry<SecretKey> for LoRaArpRegistry<PacketT, SecretKey, NonceSize>
    where PacketT: Packet + Into<Vec<u8>> + From<Vec<u8>>,
          SecretKey: KeySizeUser,
          NonceSize: ArrayLength<u8> + Clone + Copy + Eq + Hash
{
    fn set_addr(&mut self, addr: u8) {
        self.address = addr;
    }

    fn set_secret_key(&mut self, secret_key: SecretKey) {
        self.secret_key = Some(secret_key);
    }

    fn set_device_status(&mut self, device_status: DeviceStatus) {
        self.device_status = device_status;
    }

    fn set_iv_expiration_duration(&mut self, duration: Duration) {
        self.iv_expiration_duration = Some(duration);
    }
}



