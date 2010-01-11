/***************************************************************************
 *   Copyright (C) 2005-2007 by NetSieben Technologies INC                 *
 *   Author: Andrew Useckas                                                *
 *   Email: andrew@netsieben.com                                           *
 *                                                                         *
 *   Windows Port and bugfixes: Keef Aragon <keef@netsieben.com>           *
 *                                                                         *
 *   This program may be distributed under the terms of the Q Public       *
 *   License as defined by Trolltech AS of Norway and appearing in the     *
 *   file LICENSE.QPL included in the packaging of this file.              *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 ***************************************************************************/

#ifndef NE7SSH_STRING_H
#define NE7SSH_STRING_H

#include "ne7ssh_types.h"
#include <botan/bigint.h>
 
/**
@author Andrew Useckas
*/
class ne7ssh_string
{
private:
  Botan::byte **positions;
  uint32 parts;
  uint32 currentPart;

protected:
  Botan::SecureVector<Botan::byte> buffer;

public:
    /**
     * ne7ssh_string class default consturctor.
     *<p> Zeros out 'positions' and 'parts'.
     */
    ne7ssh_string();

    /**
     * ne7ssh_string class consturctor.
     *<p> Takes a vector as an argument and places the data into 'buffer'.
     * @param var Reference to a vector containing a string.
     * @param position Position in the vector to start reading from. If '0', the entire vector is dumped into 'buffer'.
     */
    ne7ssh_string(Botan::SecureVector<Botan::byte>& var, uint32 position);

    /**
     * Same as above costructor, but instead of vector it works with a string (const char*).
     * @param var  Pointer to a string terminated by '/0'.
     * @param position  Read from this position onwards.
     */
    ne7ssh_string(const char* var, uint32 position);

    /**
     * ne7ssh_string class destructor.
     */
    virtual ~ne7ssh_string();

    /**
     * Zeros out the buffer
     */
    void clear() { buffer.destroy(); }

    /**
     * Adds a string to the buffer. 
     * <p>Adds an integer representing the length of the string, converted to the network format, before the actual string data.
     * Required by SSH protocol specifications.
     * @param str pointer to a string.
     */
    void addString (const char* str);

    /**
     * Reads content of an ASCII file and appends it to the buffer.
     * @param filename Full path to ASCII file.
     * @return True is file is successfully read. Otherwise False is returned.
     */
    bool addFile (const char* filename);

    /**
     * Adds a byte stream to the buffer.
     * @param buff Pointer to the byte stream.
     * @param len Length of the byte stream.
     */
    void addBytes (const Botan::byte* buff, uint32 len);

    /**
     * Adds a vector to the buffer.
     * @param secvec Reference to the vector.
     */
    void addVector (Botan::SecureVector<Botan::byte>& secvec);

    /**
     * Adds a vector to the buffer. 
     * <p>Adds an integer representing the length of the vector, converted to the network format, before the actual data.
     * Required by SSH protocol specifications.
     * @param vector Reference to a vector.
     */
    void addVectorField (const Botan::SecureVector<Botan::byte>& vector);

    /**
     * Adds a single character to the buffer.
     * @param ch a single character.
     */
    void addChar (const char ch);

    /**
     * Adds a single integer to the buffer. 
     *<p>Integer is converted to network format as required by SSH protocol specifications.
     * @param var a single integer.
     */
    void addInt (const uint32 var);

    /**
     * Adds a BigInt variable to the buffer.
     *<p>BigInt is first converted to a vector, then the integer, representing length of the vector is converted to the network format.
     * Converted ingeger is added to the buffer, followed by the vector.
     * @param bn Reference to BigInt variable.
     */
    void addBigInt (const Botan::BigInt& bn);

    /**
     * Returns the buffer as a vector.
     * @return Reference to the 'buffer' vector.
     */
    virtual Botan::SecureVector<Botan::byte> &value () { return buffer; }

    /**
     * Returns current length of the buffer.
     * @return Length of the buffer.
     */
    uint32 length () { return buffer.size(); }

    /**
     * Extracts a single string from the payload field of SSH packet.
     * @param result Reference to a buffer where the result will be stored.
     * @return True if string field was found and successfully parsed, otherwise false is returned.
     */
    bool getString (Botan::SecureVector<Botan::byte>& result);

    /**
     * Extracts a single BigInt variable from the payload field of SSH packet.
     * @param result Reference to a BigInt variable where the result will be stored.
     * @return True if BigInt field was found and successfully parsed, otherwise false is returned.
     */
    bool getBigInt (Botan::BigInt& result);

    /**
     * Extracts a single unsigned integer (uint32) from the payload field of SSH packet.
     * @return The integer extracted from the next 4 bytes of the payload.
     */
    uint32 getInt ();

    /**
     * Extracts a single byte from tje payload field of SSH packet.
     * @return A byte extracted from thenext byte of the payload.
     */
    Botan::byte getByte ();

    /**
     * Splits the buffer into strings separated by null character.
     * @param token Searches for this character in the buffer, replaces it with null and creates a part index.
     */
    void split (const char token);

    /**
     * Returns to the first part.
     */
    void resetParts () { currentPart = 0; }

    /**
     * Returns the next part.
     * @return NULL terminated string, or 0 if this is the last part.
     */
    char* nextPart ();

    /**
     * Chops bytes off of the end of the buffer.
     * @param nBytes How many bytes to chop off the end of the buffer.
     */
    void chop (uint32 nBytes);

    /**
     * Converts BigInt into vector
     * <p> For internal use only
     * @param result Reference to vector where the converted result will be dumped.
     * @param bi Reference to BigInt to convert.
     */
    static void bn2vector (Botan::SecureVector<Botan::byte>& result, const Botan::BigInt& bi);

};

#endif
