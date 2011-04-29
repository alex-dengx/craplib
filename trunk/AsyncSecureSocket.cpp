// This code is licensed under New BSD Licence. For details see project page at
// http://code.google.com/p/craplib/source/checkout

#include "AsyncSecureSocket.h"

void SecureRWSocket::onDisconnect(const RWSocket& sock) {
    
}

void SecureRWSocket::onCanRead(const RWSocket& sock) {
    // Got encrypted data on Socket
    // Use ssl_ (writeEncrypted)
    // to get plain data
    // Using ssl_ (readDecrypted)
}

void SecureRWSocket::onCanWrite(const RWSocket& sock) {
    // Need to put some more plain data into ssl_ (writeDecrypted)
    // and then
    // Can get some more encrypted data from ssl_ (readEncrypted)
    // and put it into the sock
}
