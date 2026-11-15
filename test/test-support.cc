#include "test-support.h"

#include <system_error>

#include <cerrno>

#ifdef _WIN32
#  include <io.h>
#else
#  include <unistd.h>
#endif

namespace {
auto descriptor(std::FILE* file) -> int {
#ifdef _WIN32
  return _fileno(file);
#else
  return fileno(file);
#endif
}
auto duplicate(int fd) -> int {
#ifdef _WIN32
  return _dup(fd);
#else
  return dup(fd);
#endif
}
auto redirect_to(int from, int to) -> int {
#ifdef _WIN32
  return _dup2(from, to);
#else
  return dup2(from, to);
#endif
}
void close_descriptor(int fd) {
#ifdef _WIN32
  (void)_close(fd);
#else
  (void)close(fd);
#endif
}
[[noreturn]] void fail(int code, const char* message) {
  throw std::system_error(code, std::generic_category(), message);
}
}  // namespace

namespace tjg::test {
output_redirect::output_redirect(std::FILE* file) : _file{file} {
  if (std::fflush(file) != 0) fail(errno, "flush output");
  _capture = std::tmpfile();
  if (!_capture) fail(errno, "create capture file");
  _saved = duplicate(descriptor(file));
  if (_saved == -1 ||
      redirect_to(descriptor(_capture), descriptor(file)) == -1) {
    const auto error = errno;
    if (_saved != -1) close_descriptor(_saved);
    std::fclose(_capture);
    fail(error, "redirect output");
  }
}

output_redirect::~output_redirect() {
  if (_saved != -1) {
    (void)std::fflush(_file);
    (void)redirect_to(_saved, descriptor(_file));
    close_descriptor(_saved);
  }
  if (_capture) std::fclose(_capture);
}

auto output_redirect::restore_and_read() -> std::string {
  if (std::fflush(_file) != 0) fail(errno, "flush captured output");
  if (redirect_to(_saved, descriptor(_file)) == -1)
    fail(errno, "restore output");
  close_descriptor(_saved);
  _saved = -1;
  if (std::fseek(_capture, 0, SEEK_SET) != 0) fail(errno, "rewind capture");
  auto result = std::string{};
  char buffer[4096];
  for (;;) {
    const auto count = std::fread(buffer, 1, sizeof(buffer), _capture);
    result.append(buffer, count);
    if (count != sizeof(buffer)) {
      if (std::ferror(_capture)) fail(errno, "read capture");
      return result;
    }
  }
}
}  // namespace tjg::test
