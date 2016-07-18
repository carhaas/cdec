#ifndef EXTRACCT_REQUEST_H
#define	EXTRACCT_REQUEST_H


using namespace std;

namespace extractor {

  /**
 * Data structure that can request grammar from the extract_daemon
 */
class Requester {
 public:
  // Sets up a Requester that will connect to the supplied url
  Requester(const char *url, int to = 10000);

  ~Requester();

  const char* request_for_sentence(const char *sentence);

 private:
  int socket;
  int timeout;
};

} // namespace extractor

#endif