template <typename Char, typename OutputIt> class sgr_scope {
  OutputIt& _out;
  text_style& _state;
  text_style _saved;
  int _exceptions = std::uncaught_exceptions();
  basic_memory_buffer<Char, sgr_max_length> _sgr;

  void transition(text_style target) {
    _sgr.clear();
    append_sgr_transition(_sgr, target, _state);
    for (const auto ch : std::basic_string_view<Char>{_sgr.data(), _sgr.size()})
    {
      *_out = ch;
      ++_out;
    }
    _state = target.absolute(false);
  }

 public:
  sgr_scope(OutputIt& out, text_style style)
      : _out{out}, _state{get_style_state()}, _saved{_state} {
    if (!style.is_absolute()) style.underlay(_saved);
    transition(style);
  }

  ~sgr_scope() noexcept {
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
    try {
#endif
      // A throwing nested formatter may have emitted SGR before restoring its
      // logical state. Rebuild the entry style when unwinding in that case.
      transition(text_style{_saved}.absolute(
          std::uncaught_exceptions() > _exceptions));
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
    } catch (...) {
      // Closing SGR is best effort.
    }
#endif
    _state = _saved;
  }

  sgr_scope(const sgr_scope&) = delete;
  void operator=(const sgr_scope&) = delete;
}; // sgr_scope
