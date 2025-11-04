#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string>
#include <string_view>
#include <vector>
#include <iostream>
#include <openssl/evp.h>
#include <openssl/rand.h>

using namespace std;

string bytesvector_to_hex(const vector<unsigned char> & bytes)
{
    string hex_string;
    for (unsigned char c : bytes)
    {
        int first_in_pair = (c >> 4) & 0x0F;
        int second_in_pair = c & 0x0F;

        hex_string.push_back((char)((first_in_pair < 10) ? ('0' + first_in_pair) : ('a' + first_in_pair - 10)));
        hex_string.push_back((char)((second_in_pair < 10) ? ('0' + second_in_pair) : ('a' + second_in_pair - 10)));
    }
    return hex_string;
}

int get_random_size()
{
    int random_number = 0;
    vector<unsigned char> buffer(sizeof(int));
    RAND_bytes(buffer.data(), sizeof(int));

    for (size_t i = 0; i < sizeof(int); ++i)
        random_number += static_cast<int>(buffer[i]) << (8 * i);

    int size_string = (abs(random_number) % 25) + 8;
    return size_string;
}

vector<unsigned char> get_random_string()
{
    int size_string = get_random_size();
    std::vector<unsigned char> buffer(size_string);
    RAND_bytes(buffer.data(), size_string);

    return buffer;
}

vector<unsigned char> create_bitmask(int bits_amount)
{
    vector<unsigned char> mask;
    int full_bytes = bits_amount / 8;
    int part_bytes = bits_amount % 8;

    for(int i = 0; i < full_bytes; i++)
        mask.push_back(0x00);

    if (part_bytes > 0) {
        unsigned char temp = 0xFF;
        temp >>= part_bytes;
        mask.push_back(temp);
    }

    if(full_bytes == 0 && part_bytes == 0)
        mask.push_back(0x80);

    return mask;
}


bool findHash (int numberZeroBits, string & outputMessage, string & outputHash)
{
    if(numberZeroBits < 0 || numberZeroBits > 512) return 0;
    vector<unsigned char> random_string = get_random_string();
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_size = 0;
    OpenSSL_add_all_digests();
    vector<unsigned char> bitmask = create_bitmask(numberZeroBits);

    while (true)
    {
        EVP_MD_CTX* context = EVP_MD_CTX_new();
        if (context == nullptr) return false;
        if(!EVP_DigestInit_ex(context, EVP_sha512(), nullptr)) return false;
        if(!EVP_DigestUpdate(context, random_string.data(), random_string.size())) return false;
        if(!EVP_DigestFinal_ex(context, hash, &hash_size)) return false;

        vector<unsigned char> hash_vector;
        for (unsigned char i : hash)
            hash_vector.push_back(i);

        string hash_string = bytesvector_to_hex(hash_vector);
        bool found_hash = true;

        for(int i = 0; i < (int)bitmask.size(); i++)
        {
            if((bitmask[i] & hash_vector[i]) != hash_vector[i])
            {
                found_hash = false;
                break;
            }

            if(numberZeroBits == 0 && (bitmask[i] & hash_vector[i]) == 0)
            {
                found_hash = false;
                break;
            }
        }

        if(found_hash)
        {
            outputMessage = bytesvector_to_hex(random_string);
            outputHash = hash_string;
            EVP_MD_CTX_free(context);
            return true;
        }

        random_string.clear();
        random_string.swap(hash_vector);
        EVP_MD_CTX_free(context);
    }
}


int checkHash(int bits, const std::string & hash)
{
    int bytesToCheck = bits / 8;
    int extraBits = bits % 8;

    for (int i = 0; i < bytesToCheck; ++i)
    {
        std::string byteString = hash.substr(i * 2, 2);
        auto byte = static_cast<unsigned char>(std::stoul(byteString, nullptr, 16));

        if (byte != 0) return 0;
    }

    if (extraBits > 0)
    {
        std::string byteString = hash.substr(bytesToCheck * 2, 2);
        auto byte = static_cast<unsigned char>(std::stoul(byteString, nullptr, 16));
        auto mask = static_cast<unsigned char>(0xFF << (8 - extraBits));

        if ((byte & mask) != 0) return 0;
    }

    return 1;
}


int main ()
{
    string hash, message;

	cout << "Trying to find a hash strating with 0 zeros" << endl;
    findHash(0,message, hash);
    assert(findHash(0, message, hash));
    assert(!message.empty() && !hash.empty() && checkHash(0, hash));
	cout << "Hash found: " << hash << endl << "String is: " << message << endl << endl;
    message.clear();
    hash.clear();

	cout << "Trying to find a hash strating with 1 zero bit" << endl;
    assert(findHash(1, message, hash));
    assert(!message.empty() && !hash.empty() && checkHash(1, hash));
	cout << "Hash found: " << hash << endl << "String is: " << message << endl << endl;
    message.clear();
    hash.clear();

	cout << "Trying to find a hash strating with 2 zero bits" << endl;
    assert(findHash(2, message, hash));
    assert(!message.empty() && !hash.empty() && checkHash(2, hash));
	cout << "Hash found: " << hash << endl << "String is: " << message << endl << endl;
    message.clear();
    hash.clear();

	cout << "Trying to find a hash strating with 3 zero bits" << endl;
    assert(findHash(3, message, hash));
    assert(!message.empty() && !hash.empty() && checkHash(3, hash));
	cout << "Hash found: " << hash << endl << "String is: " << message << endl;
    message.clear();
    hash.clear();

    assert(!findHash(-1, message, hash));
    return 0;
}


