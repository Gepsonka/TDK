pub trait Queue<PacketT> {
    fn get_top_item(&mut self) -> Option<PacketT>;
    fn pop(&mut self) -> Option<PacketT>;
    fn push(&mut self, item: PacketT);
    fn is_empty(&self) -> bool;
    fn len(&self) -> usize;
}
