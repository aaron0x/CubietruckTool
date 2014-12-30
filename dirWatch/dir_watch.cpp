#include <sys/inotify.h>
#include <signal.h>
#include <limits.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <stdexcept>
#include <sstream>
#include <errno.h>

using namespace std;

bool run = true;

void   subscribeSignals();
void   quitMonitor(int sigNum);
int    initializeInotify(const char* path);
void   keepMonitorDir(int fd);
string toStr(int number);

int main(int argc, char** arvg) {
   if (argc != 2) {
      cerr << "Usage: program dir_path\n";
      return EXIT_FAILURE;
   }

   try {
      int fd = initializeInotify(arvg[1]);
      subscribeSignals();
      keepMonitorDir(fd);
      close(fd);
   } catch (const exception& e) {
      cerr << e.what();
      return EXIT_FAILURE;  
   }

   cout << "quit gracefully\n";

   return EXIT_SUCCESS;
}

void subscribeSignals() {
   signal(SIGINT,  quitMonitor);
   signal(SIGHUP,  quitMonitor);
   signal(SIGTERM, quitMonitor);
   signal(SIGKILL, quitMonitor);   
}

void quitMonitor(int sigNum) {
   cout << "sig receiver " << sigNum << endl;
   run = false;
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

void keepMonitorDir(int fd) {
   while (run) {
      char buf[sizeof(struct inotify_event) + NAME_MAX + 1] = "";
      int rc = read(fd, buf, sizeof(buf));
      if (rc == EIO) {
         throw runtime_error("read inotify fd error");
      }

      // turn on led.
      cout << "qqq\n";
   }

   cout << "quit monitor\n";
}

string toStr(int number) {
   stringstream ss;
   ss << number;
   return ss.str();
}
