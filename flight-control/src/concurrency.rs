use std::sync::{Arc, Mutex};



pub struct SharedState <T> {
    pub data: Arc<Mutex<T>>
}