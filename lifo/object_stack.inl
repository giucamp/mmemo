


namespace memo
{

	// ObjectStack::is_initialized
	MEMO_INLINE bool ObjectStack::is_initialized() const
	{
		return m_target_allocator != nullptr;
	}

	// ObjectStack::destructor
	MEMO_INLINE ObjectStack::~ObjectStack()
	{
		uninit();
	}

} // namespace memo
