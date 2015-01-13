#include <sys/inotify.h>
#include <signal.h>
#include <limits.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <stdexcept>
#include <sstream>
#include <cerrno>
#include <unistd.h>
#include <system_error>

using namespace std;

bool volatile run = true;

// set values of -d/-f to dir/command.
bool parseParameters(int argc, char** const argv, char** dir, char** command);

void subscribeSignals();

int initializeInotify(const char* path);

// start to watch dir infinitely. excute command when dir changing.
void keepMonitorDir(int fd, const char* command);

int main(int argc, char** argv) {
   char* dir     = NULL;
   char* command = NULL;

   if (!parseParameters(argc, argv, &dir, &command)) {
      cerr << "Usage: program -d dir_path -c command_for_dir_change\n";
      return EXIT_FAILURE;
   }

   try {
      int fd = initializeInotify(dir);
      subscribeSignals();
      keepMonitorDir(fd, command);
      close(fd);
   } catch (const system_error& s) {
      cerr << s.what() << endl;
      cerr << s.code() << endl;
    
   } catch (const exception& e) {
      cerr << e.what() << endl;
      return EXIT_FAILURE;  
   }

   return EXIT_SUCCESS;
}

bool parseParameters(int argc, char** const argv, char** dir, char** command) {
   int   o = -1;
   char* d = NULL;
   char* c = NULL;

   while((o = getopt(argc, argv, "d:c:")) != -1) {
      switch(o) {
         case 'd':
            d = optarg;
            break;
         case 'c':
            c = optarg;
            break;
         default:
            return false;
            break;
      }
   }

   if (!d || !c) {
      return false;
   }

   *dir     = d;
   *command = c;

   return true;
}

// Just catch signal.
void subscribeSignals() {
   auto handle = [](int sigNum) { run = false; };
   signal(SIGINT,  handle);
   signal(SIGHUP,  handle);
   signal(SIGTERM, handle);
   signal(SIGKILL, handle);   
}

int initializeInotify(const char* path) {
   int fd = inotify_init();
   if (fd == -1) {
      throw system_error(errno, system_category(), "inotify init failed");
   }

   if (inotify_add_watch(fd, path, IN_MODIFY | IN_CREATE) == -1) {
      close(fd);
      throw system_error(errno, system_category(), "inotify_add_watch failed");
   }

   return fd;
}

void keepMonitorDir(int fd, const char* command) {
    while (run) {
      char buf[sizeof(struct inotify_event) + NAME_MAX + 1] = "";

      if (read(fd, buf, sizeof(buf)) > 0) {
         int commandRC = system(command);
         if (commandRC == -1) {
            throw runtime_error("system return -1");
         }

         int realRC = WEXITSTATUS(commandRC);
         if (realRC == 127) {
            throw runtime_error(string("command error: ") + command);
         }
         if (realRC) {
            throw system_error(WEXITSTATUS(realRC), generic_category(), "command faile");
         }          
      } else if (errno != EINTR) {
         throw system_error(errno, system_category(), "read error");
      }
   }
}

