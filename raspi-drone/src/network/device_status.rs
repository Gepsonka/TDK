
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum DeviceStatus {
    Unknown,
    RequestingAddress,
    Unauthorized,
    KeyExchangeInitiated,
    Online
}
