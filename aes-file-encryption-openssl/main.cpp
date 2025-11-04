#include <cstdint>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <unistd.h>
#include <string>
#include <vector>
#include <fstream>
#include <cassert>
#include <cstring>

#include <openssl/evp.h>
#include <openssl/rand.h>

using namespace std;

struct crypto_config {
    const char * m_crypto_function;
    std::unique_ptr<uint8_t[]> m_key;
    std::unique_ptr<uint8_t[]> m_IV;
    size_t m_key_len;
    size_t m_IV_len;
};

bool check_config(crypto_config & config, const EVP_CIPHER * cypher_name)
{
    int requested_key_size = EVP_CIPHER_key_length(cypher_name);
    int requested_iv_size = EVP_CIPHER_iv_length(cypher_name);

    if(config.m_key == nullptr || (int)config.m_key_len < requested_key_size)
    {
        unique_ptr<uint8_t[]> new_key = std::make_unique<uint8_t[]>(requested_key_size);
        if (RAND_bytes(new_key.get(), requested_key_size) != 1) return false;

        config.m_key = std::move(new_key);
        config.m_key_len = requested_key_size;
    }

    if(requested_iv_size != 0 && (config.m_IV == nullptr || (int)config.m_IV_len < requested_iv_size))
    {
        unique_ptr<uint8_t[]> new_vector = std::make_unique<uint8_t[]>(requested_iv_size);
        if (RAND_bytes(new_vector.get(), requested_iv_size) != 1) return false;

        config.m_IV = std::move(new_vector);
        config.m_IV_len = requested_iv_size;
    }
    return true;
}

bool encrypt_data(const std::string & in_filename, const std::string & out_filename, crypto_config & config) {
    OpenSSL_add_all_ciphers();

    if(in_filename.empty() || out_filename.empty() || config.m_crypto_function == nullptr)
        return false;

    ifstream input_file(in_filename, std::ios::binary);
    ofstream output_file(out_filename, std::ios::binary);
    if (!(input_file.is_open()) || input_file.fail() || !(output_file.is_open()) || output_file.fail()) {
        input_file.close();
        output_file.close();
        return false;
    }

    const EVP_CIPHER * cypher_name = EVP_get_cipherbyname(config.m_crypto_function);
    if(!cypher_name || !check_config(config, cypher_name)) {
        input_file.close();
        output_file.close();
        return false;
    }

    EVP_CIPHER_CTX * ctx = EVP_CIPHER_CTX_new();
    if(ctx == nullptr) {
        input_file.close();
        output_file.close();
        return false;
    }

    if(!EVP_EncryptInit_ex(ctx, cypher_name, nullptr, config.m_key.get(), config.m_IV.get())) {
        EVP_CIPHER_CTX_free(ctx);

        input_file.close();
        output_file.close();
        return false;
    }

    int header_size = 18;
    vector<char> header(header_size);

    input_file.read(header.data(), header_size);
    if(input_file.bad())
    {
        EVP_CIPHER_CTX_free(ctx);
        input_file.close();
        output_file.close();
        return false;
    }

    if(input_file.gcount() < header_size)
    {
        EVP_CIPHER_CTX_free(ctx);
        input_file.close();
        output_file.close();
        return false;
    }

    output_file.write(header.data(), (int)header.size());
    if(output_file.bad())
    {
        EVP_CIPHER_CTX_free(ctx);
        input_file.close();
        output_file.close();
        return false;
    }

    int block_size =  EVP_CIPHER_block_size(cypher_name);
    std::vector<unsigned char> block(block_size);
    std::vector<unsigned char> encrypted_block(block.size() + block_size);
    int out_len = 0;

    while (!input_file.eof())
    {
        input_file.read((char*)block.data(), block_size);
        if(input_file.bad())
        {
            EVP_CIPHER_CTX_free(ctx);
            input_file.close();
            output_file.close();
            return false;
        }

        int bytes_read = (int)input_file.gcount();
        if (!EVP_EncryptUpdate(ctx, encrypted_block.data(), &out_len, block.data(),  bytes_read))
        {
            EVP_CIPHER_CTX_free(ctx);
            input_file.close();
            output_file.close();
            return false;
        }

        output_file.write((char*)encrypted_block.data(), out_len);
        if(output_file.bad())
        {
            EVP_CIPHER_CTX_free(ctx);
            input_file.close();
            output_file.close();
            return false;
        }
    }

    if (!EVP_EncryptFinal_ex(ctx, encrypted_block.data(), &out_len))
    {
        EVP_CIPHER_CTX_free(ctx);
        input_file.close();
        output_file.close();
        return false;
    }

    output_file.write((char*)encrypted_block.data(), out_len);
    if(output_file.bad())
    {
        EVP_CIPHER_CTX_free(ctx);
        input_file.close();
        output_file.close();
        return false;
    }

    EVP_CIPHER_CTX_free(ctx);
    input_file.close();
    output_file.close();

    return true;
}

bool decrypt_data(const std::string & in_filename, const std::string & out_filename, crypto_config & config) {
    OpenSSL_add_all_ciphers();

    if(in_filename.empty() || out_filename.empty() || config.m_crypto_function == nullptr) return false;

    ifstream input_file(in_filename, std::ios::binary);
    ofstream output_file(out_filename, std::ios::binary);
    if (!(input_file.is_open()) || input_file.fail() || !(output_file.is_open()) || output_file.fail())
    {
        input_file.close();
        output_file.close();
        return false;
    }

    const EVP_CIPHER * cypher_name = EVP_get_cipherbyname(config.m_crypto_function);
    if(!cypher_name ||
       (config.m_key == nullptr || (int)config.m_key_len < EVP_CIPHER_key_length(cypher_name)) ||
       (EVP_CIPHER_iv_length(cypher_name) != 0 && (config.m_IV == nullptr || (int)config.m_IV_len < EVP_CIPHER_iv_length(cypher_name))) ||
       !check_config(config, cypher_name)) {
        input_file.close();
        output_file.close();
        return false;
    }

    EVP_CIPHER_CTX * ctx = EVP_CIPHER_CTX_new();
    if(ctx == nullptr)
    {
        input_file.close();
        output_file.close();
        return false;
    }

    if(!EVP_DecryptInit_ex(ctx, cypher_name, nullptr, config.m_key.get(), config.m_IV.get()))
    {
        EVP_CIPHER_CTX_free(ctx);
        input_file.close();
        output_file.close();
        return false;
    }

    int header_size = 18;
    vector<char> header(header_size);

    input_file.read(header.data(), header_size);
    if(input_file.bad())
    {
        EVP_CIPHER_CTX_free(ctx);
        input_file.close();
        output_file.close();
        return false;
    }

    if(input_file.gcount() < header_size)
    {
        EVP_CIPHER_CTX_free(ctx);
        input_file.close();
        output_file.close();
        return false;
    }

    output_file.write(header.data(), (int)header.size());
    if(output_file.bad())
    {
        EVP_CIPHER_CTX_free(ctx);
        input_file.close();
        output_file.close();
        return false;
    }

    int block_size = EVP_CIPHER_block_size(cypher_name);
    std::vector<unsigned char> block(block_size);
    std::vector<unsigned char> encrypted_block(block.size() + block_size);
    int out_len = 0;

    while (!input_file.eof())
    {
        input_file.read((char*)block.data(), block_size);
        if(input_file.bad())
        {
            EVP_CIPHER_CTX_free(ctx);
            input_file.close();
            output_file.close();
            return false;
        }

        int bytes_read = (int)input_file.gcount();
        if (!EVP_DecryptUpdate(ctx, encrypted_block.data(), &out_len, block.data(), bytes_read))
        {
            EVP_CIPHER_CTX_free(ctx);
            input_file.close();
            output_file.close();
            return false;
        }

        output_file.write((char*)encrypted_block.data(), out_len);
        if(output_file.bad())
        {
            EVP_CIPHER_CTX_free(ctx);
            input_file.close();
            output_file.close();
            return false;
        }
    }

    if (!EVP_DecryptFinal_ex(ctx, encrypted_block.data(), &out_len))
    {
        EVP_CIPHER_CTX_free(ctx);
        input_file.close();
        output_file.close();
        return false;
    }

    output_file.write((char*)encrypted_block.data(), out_len);
    if(output_file.bad())
    {
        EVP_CIPHER_CTX_free(ctx);
        input_file.close();
        output_file.close();
        return false;
    }

    EVP_CIPHER_CTX_free(ctx);
    input_file.close();
    output_file.close();

    return true;
}

static bool compare_files(const std::string& a, const std::string& b)
{
    std::ifstream fa(a, std::ios::binary);
    std::ifstream fb(b, std::ios::binary);
    if (!fa.is_open() || !fb.is_open())
        return false;

    constexpr std::size_t BUF_SIZE = 1u << 20;
    std::vector<unsigned char> ba(BUF_SIZE);
    std::vector<unsigned char> bb(BUF_SIZE);

    while (true)
    {
        fa.read(reinterpret_cast<char*>(ba.data()),BUF_SIZE);
        fb.read(reinterpret_cast<char*>(bb.data()),BUF_SIZE);

        const std::streamsize ra = fa.gcount();
        const std::streamsize rb = fb.gcount();

        if (ra != rb) return false;
        if (ra == 0) break;
        if (std::memcmp(ba.data(), bb.data(), static_cast<std::size_t>(ra)) != 0) return false;
        if (fa.bad() || fb.bad()) return false;
    }

    if (fa.bad() || fb.bad()) return false;

    return true;
}


int main ()
{
    crypto_config config {nullptr, nullptr, nullptr, 0, 0};

    // ECB mode
    config.m_crypto_function = "AES-128-ECB";
    config.m_key = std::make_unique<uint8_t[]>(16);
    memset(config.m_key.get(), 0, 16);
    config.m_key_len = 16;

    assert( encrypt_data  ("testfiles/homer-simpson.TGA", "testfiles/out_file.TGA", config) &&
            compare_files ("testfiles/out_file.TGA", "testfiles/homer-simpson_enc_ecb.TGA") );

    assert( decrypt_data  ("testfiles/homer-simpson_enc_ecb.TGA", "testfiles/out_file.TGA", config) &&
            compare_files ("testfiles/out_file.TGA", "testfiles/homer-simpson.TGA") );

    assert( encrypt_data  ("testfiles/UCM8.TGA", "testfiles/out_file.TGA", config) &&
            compare_files ("testfiles/out_file.TGA", "testfiles/UCM8_enc_ecb.TGA") );

    assert( decrypt_data  ("testfiles/UCM8_enc_ecb.TGA", "testfiles/out_file.TGA", config) &&
            compare_files ("testfiles/out_file.TGA", "testfiles/UCM8.TGA") );

    assert( encrypt_data  ("testfiles/image_1.TGA", "testfiles/out_file.TGA", config) &&
            compare_files ("testfiles/out_file.TGA", "testfiles/ref_1_enc_ecb.TGA") );

    assert( encrypt_data  ("testfiles/image_2.TGA", "testfiles/out_file.TGA", config) &&
            compare_files ("testfiles/out_file.TGA", "testfiles/ref_2_enc_ecb.TGA") );

    assert( decrypt_data ("testfiles/image_3_enc_ecb.TGA", "testfiles/out_file.TGA", config)  &&
            compare_files("testfiles/out_file.TGA", "testfiles/ref_3_dec_ecb.TGA") );

    assert( decrypt_data ("testfiles/image_4_enc_ecb.TGA", "testfiles/out_file.TGA", config)  &&
            compare_files("testfiles/out_file.TGA", "testfiles/ref_4_dec_ecb.TGA") );

    // CBC mode
    config.m_crypto_function = "AES-128-CBC";
    config.m_IV = std::make_unique<uint8_t[]>(16);
    config.m_IV_len = 16;
    memset(config.m_IV.get(), 0, 16);

    assert( encrypt_data  ("testfiles/UCM8.TGA", "testfiles/out_file.TGA", config) &&
            compare_files ("testfiles/out_file.TGA", "testfiles/UCM8_enc_cbc.TGA") );

    assert( decrypt_data  ("testfiles/UCM8_enc_cbc.TGA", "testfiles/out_file.TGA", config) &&
            compare_files ("testfiles/out_file.TGA", "testfiles/UCM8.TGA") );

    assert( encrypt_data  ("testfiles/homer-simpson.TGA", "testfiles/out_file.TGA", config) &&
            compare_files ("testfiles/out_file.TGA", "testfiles/homer-simpson_enc_cbc.TGA") );

    assert( decrypt_data  ("testfiles/homer-simpson_enc_cbc.TGA", "testfiles/out_file.TGA", config) &&
            compare_files ("testfiles/out_file.TGA", "testfiles/homer-simpson.TGA") );

    assert( encrypt_data  ("testfiles/image_1.TGA", "testfiles/out_file.TGA", config) &&
            compare_files ("testfiles/out_file.TGA", "testfiles/ref_5_enc_cbc.TGA") );

    assert( encrypt_data  ("testfiles/image_2.TGA", "testfiles/out_file.TGA", config) &&
            compare_files ("testfiles/out_file.TGA", "testfiles/ref_6_enc_cbc.TGA") );

    assert( decrypt_data ("testfiles/image_7_enc_cbc.TGA", "testfiles/out_file.TGA", config)  &&
            compare_files("testfiles/out_file.TGA", "testfiles/ref_7_dec_cbc.TGA") );

    assert( decrypt_data ("testfiles/image_8_enc_cbc.TGA", "testfiles/out_file.TGA", config)  &&
            compare_files("testfiles/out_file.TGA", "testfiles/ref_8_dec_cbc.TGA") );

    return 0;
}
