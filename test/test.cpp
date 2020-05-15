#include <airbag/process_error.hpp>
#include <airbag/thread_error.hpp>
#include <airbag/minidump.hpp>


airbag::process_error process_error;
airbag::minidump minidump;

int main(int, char**) {

  process_error.on_pure_call([] {
    fprintf(stderr, "Oops: pure virtual function call\n");
  });

  process_error.pre_system_failure([](airbag::system_failure const& f) {
    fprintf(stderr, "Oops: %s at %s, generating minidump at '%s'\n",
            f.title(), f.module_name(),
            minidump.directory().string().data());
    if(!minidump.generate(f))
      fprintf(stderr, "Unable to generate minidump: %s\n",
              minidump.last_error().message().data());
  });

  thread_local airbag::thread_error thread_error;
  thread_error.on_terminate([](char const* message){
    fprintf(stderr, "%s\n", message);
  });

  try {

    // Doing bad things
    *((char*)0) = -1;


  } catch(airbag::thread_error const& e) {
    fprintf(stderr, "Runtime error [%s]: %s\n", e.failure().module_name(), e.what());
    return 1;
  } catch(std::exception const& e) {
    fprintf(stderr, "Exception: %s\n", e.what());
    return 1;
  } catch(...) {
    fprintf(stderr, "Unknown exception\n");
    return 1;
  }

  return 0;
}
