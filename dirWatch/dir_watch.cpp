#include <sys/inotify.h>
#include <signal.h>
#include <limits.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <stdexcept>
#include <sstream>
#include <errno.h>
#include <unistd.h>

using namespace std;

// set values of -d/-f to dir/command.
bool parseParameters(int argc, char** const argv, char** path, char** command, bool* inDaemon);

void subscribeSignals();

int initializeInotify(const char* path);

// start to watch dir infinitely. excute command when dir changing.
void keepMonitorDir(int fd, const char* command);

string&& toStr(int number);

int main(int argc, char** argv) {
   char* path     = NULL;
   char* command  = NULL;
   bool  inDaemon = false;

   if (!parseParameters(argc, argv, &path, &command, &inDaemon)) {
      cerr << "Usage: program -d dir_path -c command_for_dir_change\n";
      return EXIT_FAILURE;
   }

   try {
      int fd = initializeInotify(path);
      subscribeSignals();
      if (inDaemon) {
         daemon(0, 0);
      }
      keepMonitorDir(fd, command);
      close(fd);
   } catch (const exception& e) {
      cerr << e.what() << endl;
      return EXIT_FAILURE;  
   }

   return EXIT_SUCCESS;
}

bool parseParameters(int argc, char** const argv, char** path, char** command, bool* inDaemon) {
   int   o = -1;
   char* p = NULL;
   char* c = NULL;
   bool  d = false;

   while((o = getopt(argc, argv, "dp:c:")) != -1) {
      switch(o) {
         case 'd':
            d = true;
            break;
         case 'p':
            p = optarg;
            break;
         case 'c':
            c = optarg;
            break;
         default:
            return false;
            break;
      }
   }

   if (!p || !c) {
      return false;
   }

   *inDaemon = d;
   *path     = p;
   *command  = c;

   return true;
}

// Just catch signal.
void subscribeSignals() {
   auto emptyHandle = [](int sigNum) {};
   signal(SIGINT,  emptyHandle);
   signal(SIGHUP,  emptyHandle);
   signal(SIGTERM, emptyHandle);
   signal(SIGKILL, emptyHandle);   
}

int initializeInotify(const char* path) {
   int fd = inotify_init();
   if (fd == -1) {
      throw runtime_error("inotify init failed: " + toStr(errno));
   }

   if (inotify_add_watch(fd, path, IN_MODIFY | IN_CREATE) == -1) {
      close(fd);
      throw runtime_error(string("inotify_add_watch failed ") + path + " " + toStr(errno));
   }

   return fd;
}

// Quit when get signal.
void keepMonitorDir(int fd, const char* command) {
    while (true) {
      char buf[sizeof(struct inotify_event) + NAME_MAX + 1] = "";

      if (read(fd, buf, sizeof(buf)) > 0) {
         int commandRc = system(command);
         if (commandRc == -1 || WEXITSTATUS(commandRc) != 0) {
            throw runtime_error("command failed, return " + toStr(WEXITSTATUS(commandRc)));
         }          
      } else if (errno == EINTR) {
         break;
      } else {
         throw runtime_error("read error, errno = " + toStr(errno));
      }
   }
}

string&& toStr(int number) {
   stringstream ss;
   ss << number;
   return move(ss.str());
}
