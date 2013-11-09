


namespace memo
{

	// LifoAllocator::is_initialized
	MEMO_INLINE bool LifoAllocator::is_initialized() const
	{
		return m_target_allocator != nullptr;
	}

	// LifoAllocator::destructor
	MEMO_INLINE LifoAllocator::~LifoAllocator()
	{
		uninit();
	}

} // namespace memo
