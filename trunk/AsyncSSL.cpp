// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#include "AsyncSSL.h"


bool SSLTube::readEncrypted(Data& bytes) {
    return false;
}

bool SSLTube::readDecrypted(Data& bytes) {
    return false;
}

size_t SSLTube::writeEncrypted(Data& bytes) {
    return 0;
}

size_t SSLTube::writeDecrypted(Data& bytes) {
    return 0;
}
