#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <cmath>
#include <chrono>
#include <cstdint>
#include <cstring>

using namespace std;

class Timer
{
private:
    using clock_t = std::chrono::high_resolution_clock;
    using second_t = std::chrono::duration<double, std::ratio<1> >;

    std::chrono::time_point<clock_t> m_beg;

public:
    Timer() : m_beg(clock_t::now())
    {
    }

    void reset()
    {
        m_beg = clock_t::now();
    }

    double elapsed() const
    {
        return std::chrono::duration_cast<second_t>(clock_t::now() - m_beg).count();
    }
};


#define MAX_CODES 100000000
#define CODE_MAX 66000
#define MAX_DIGITS 24

uint16_t maxCode;
int checkSize = 10;
uint64_t bestFullBitsSz = -1;
uint8_t bestDigitsSt[MAX_DIGITS];
uint8_t minSt[MAX_DIGITS] = {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};
uint8_t maxSt[MAX_DIGITS] = {8, 8, 8, 8, 8, 8, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};
uint32_t PSum[CODE_MAX];
uint32_t cnt[CODE_MAX];
int32_t idx = 0;
int32_t bits = 0;
uint16_t encoded[MAX_CODES];
uint32_t encodedSize = 1;
uint32_t fileIdx = 0;
uint16_t decoded[MAX_CODES];
uint32_t decodedSize = 0;
uint16_t file[MAX_CODES];
uint32_t fileSize;
uint64_t mask[MAX_DIGITS];
uint64_t pows[MAX_DIGITS];
uint64_t pref[MAX_DIGITS];

//adding bit to the encoded bitstream
void encAdd(uint16_t bit) {
    encoded[idx] |= (bit << (15-bits));
    bits++;
    if (bits == 16) {
        bits = 0;
        idx++;
        encodedSize++;
    }
}

//converting a (st)-ary number cur to a binary representation
void encode(int cur, int st) {
    if (fileIdx < checkSize) cout << cur;
    int l = ceil(log2(st));
    int x = (1 << l) - st;
    uint16_t f;
    if (cur < x) {
        f = cur;
    } else {
        f = x + (cur-x)/2;
    }
    for (int i = 0; i < l-1; i++) {
        encAdd((f>>(l-2-i))&1);
    }
    if (cur >= x) {
        encAdd((cur-x)&1);
    }
}

//encode file to encoded bitstream
void encodeFile() {
    //encoded = new uint16_t[minEncodedSize+100];
    for (int i = 0; i < 10; i++){
        encoded[i] = bestDigitsSt[i];
    }
    idx += 10;
    encodedSize += 10;
    cout << "Codes :       ";
    for (fileIdx = 0; fileIdx < fileSize; fileIdx++) {
        uint16_t cur = file[fileIdx];
        for (int j = 0; j < 24; j++) {
            uint16_t st = bestDigitsSt[j];
            if (cur == 0) {
                encode(mask[j], st);  //delimiter
                break;
            } else {
                encode((cur-1)%mask[j], st); //next digit
                cur = (cur-1)/mask[j];
            }
        }
        if (fileIdx < checkSize) cout << " ";
    }
    cout << endl;
}

//returns count of numbers from [l, r] in file
uint64_t getSum(uint32_t l, uint32_t r)
{
    uint64_t res = 0;

    res += r > maxCode ? PSum[maxCode] : PSum[r];
    res -= l == 0 ? 0 : PSum[l - 1];
    return res;
}


//best code search finds the best digit sizes and best bases for digits
void BCS(uint8_t dSt[24], uint32_t dN, uint64_t fullN,
    uint64_t incN, double cLen, uint64_t fullSz)
{
    if (dN > 6) // all digits starting from 5th = 2
    {
        if (fullN > maxCode)
        {
            dSt[dN] = 3;

            if (fullSz < bestFullBitsSz)
            {
                bestFullBitsSz = fullSz;
                for (uint32_t i = 0; i < 24; i++)
                {
                    bestDigitsSt[i] = dSt[i];
                }
            }
            return;
        }

        dSt[dN] = 3;
        uint64_t newSz = fullSz + (cLen + log2(3)) * getSum(fullN, fullN + incN - 1);
        BCS(dSt, dN + 1, fullN + incN, incN * 2, cLen + 2, newSz);
    }
    else // lines 4-7 from Alg. 3 i the paper
        for(int i = minSt[dN]; i <= maxSt[dN]; i++) {
            dSt[dN] = i;
            uint64_t newSz = fullSz + (cLen + log2(i)) * getSum(fullN, fullN + incN - 1);
            BCS(dSt, dN + 1, fullN + incN, incN * (i-1), cLen + log2(i), newSz);
        }
}

//calculate prefix sums for BCS and masks of received from BCS digits
void preCalc() {
    for (int i = 0; i < fileSize; i++) {
        maxCode = max(maxCode, file[i]);
    }
    for (int i = 0; i < fileSize; i++) {
        PSum[file[i]]++;
        cnt[file[i]]++;
    }
    for (int i = 1; i <= maxCode; i++) {
        PSum[i] += PSum[i-1];
    }
    uint8_t digitSt[24];
    memset(digitSt, 3, sizeof(uint8_t) * 24);
    for (int i = 0; i < 24; i++) {
        bestDigitsSt[i] = 3;
    }
    BCS(digitSt, 0, 0, 1, 0, 0);

    for (int i = 0; i < 24; i++) {
        mask[i] = (uint32_t)bestDigitsSt[i]-1;
    }
}


uint32_t decodeIdx = 10;
uint16_t bitPos = 0;

//gets next bit from the encoded bitstream
uint16_t getBit() {
    uint16_t bit = (encoded[decodeIdx] >> (15 - bitPos)) & 1;
    bitPos++;
    if (bitPos == 16) {
        bitPos = 0;
        decodeIdx++;
    }
    //if (decodedSize < checkSize) cout << bit;
    return bit;
};

//decodes next (st)-ary digit in encoded bitstream
uint16_t decode(uint16_t st) {
    int l = ceil(log2(st));
    int x = (1 << l) - st;
    uint16_t f = 0;

    for (int i = 0; i < l - 1; i++) {
        f <<= 1;
        f += getBit();
    }

    if (f >= x) {
        f <<= 1;
        f += getBit();
        f -= x;
    }
    //if (decodedSize < 10) cout << endl << "Decoded : " << f << endl;
    return f;
};


//decodes file from encoded bitstream
void decodeFile() {

    decodedSize = 0;

    while (decodeIdx <= encodedSize) {
        uint16_t cur = 0;
        //decode next BCMix digit
        for (int j = 0; j < 24; j++) {
            uint16_t st = bestDigitsSt[j];

            uint16_t f = decode(st);
            //if (decodedSize < checkSize) cout << "val: " << val << endl;

            //end of number
            if (f == mask[j]) {
                cur += pref[j];
                //if (decodedSize < 10) cout << "Add : " << pref[j] << endl;
                break;
            }
            cur += f*pows[j];
            //if (decodedSize < 10) cout << "Add : " <<  val*pows[j] << endl;
        }

        //if (decodedSize < checkSize) cout << endl << "Decoded : " << cur << endl;
        decoded[decodedSize++] = cur;
    }


}

void check() {
    cout << "Checking..." << endl;
    bool diff = false;
    for (int i = 0; i < decodedSize; i++) {
        if (decoded[i] != file[i]) {
            cout << i << " " << decoded[i] << " " << file[i] << endl;
            diff = true;
        }
    }
    if (diff) {
        cout << "Failed" << endl;
    } else {
        cout << "Ok" << endl;
    }
}

int main() {

    FILE* in = fopen("resources/data10", "rb");

    fseek(in, 0, SEEK_END);
    fileSize = ftell(in) / sizeof(uint16_t);
    fseek(in, 0, SEEK_SET);
    fread(file, sizeof(uint16_t), fileSize, in);

    fclose(in);

    preCalc();

    cout << "Digit base : ";
    for (int i = 0; i < 24; i++) {
        cout << (int)bestDigitsSt[i] << " ";
    }

    cout << endl;

    cout << "Input file  : ";
    for (uint32_t i = 0; i < checkSize; i++) {
        cout << file[i] << " ";
    }
    cout << endl;

    pows[0] = 1;
    pref[0] = 0;
    for (int i = 1; i < 24; i++) {
        pows[i] = pows[i-1]*mask[i-1];
        pref[i] = pref[i-1]+pows[i-1];
    }
    cout << endl;

    Timer t1;

    encodeFile();

    cout << "Encode time: " << t1.elapsed() << endl;


    cout << "Encoded file: ";
    for (uint32_t i = 10; i < checkSize+10; i++) {
        for (int j = 0; j < 16; j++) {
            cout << ((encoded[i]&(1<<15-j))>>(15-j));
        }
    }
    cout << endl;

    Timer t2;

    decodeFile();

    cout << "Decode time: " << t2.elapsed() << endl;

    cout << "Decoded file: ";
    for (uint32_t i = 0; i < checkSize; i++) {
        cout << decoded[i] << " ";
    }
    cout << endl;

    cout << "Input file size (bytes): " << fileSize*2 << "\n" << "Encoded file size (bytes): " << encodedSize*2-20 << endl;
    cout << "BestEncodedSize (bytes): " << bestFullBitsSz/8 << endl;

    cout << "Decoded file size (bytes) : " << decodedSize*2 << endl;
    check();

    FILE* out = fopen("decoded", "wb");
    fwrite(decoded, sizeof(uint16_t), decodedSize, out);

    fclose(out);

    return 0;
}