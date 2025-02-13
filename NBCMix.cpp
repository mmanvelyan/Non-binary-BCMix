#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <cmath>
#include <cstdint>
#include <cstring>

using namespace std;

#define MAX_CODES 100000000
#define CODE_MAX 66000
#define MAX_DIGITS 24

uint16_t maxCode;
int checkSize = 10;
uint64_t bestFullBitsSz = -1;
uint8_t bestDigitsSz[MAX_DIGITS];
uint8_t bestDigitsSt[MAX_DIGITS];
uint8_t shortestDigitLen = 2;
uint8_t minSt[MAX_DIGITS] = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
uint8_t maxSt[MAX_DIGITS] = {5, 5, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
uint8_t longestDigitLen[MAX_DIGITS] = {4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
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


int getBitCount(uint16_t cur, uint8_t dSz[MAX_DIGITS], uint8_t dSt[MAX_DIGITS]){
    int res = 0;
    for (int j = 0; j < 24; j++) {
        uint16_t st = dSt[j];
        uint16_t sz = dSz[j];
        uint64_t maskj = pow(st, sz)-1;
        int l = ceil(log2(st));
        int x = (1 << l) - st;
        int y = st - x;
        if (cur == 0) {
            res += sz * l;
            break;
        } else {
            uint16_t cur1 = (cur-1)%maskj;  //next digit
            for (int i = 0; i < sz; i++) {
                if (cur1%st < x){
                    res += l-1;
                } else {
                    res += l;
                }
                cur1 /= st;
            }
            cur = (cur-1)/maskj;
        }
    }
    return res;
}

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
    //if (fileIdx < checkSize) cout << cur << endl;
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

//Encode number (cur) to BCMix digit of size (sz) in base (st)
void encodeSt(uint16_t cur, uint16_t st, uint16_t sz) {
    //if (fileIdx < checkSize) cout << "cur : " << cur << endl;
    uint16_t enc[10];
    for (int i = 0; i < sz; i++) {
        enc[sz-i-1] = cur%st;
        cur /= st;
    }
    for (int i = 0; i < sz; i++){
        if (fileIdx < checkSize) cout << (int)enc[i];
        encode(enc[i], st);
    }
    if (fileIdx < checkSize) cout << " ";
}

//encode file to encoded bitstream
void encodeFile() {
    //encoded = new uint16_t[minEncodedSize+100];
    for (int i = 0; i < 5; i++){
        encoded[i] = bestDigitsSz[i];
        encoded[i+5] = bestDigitsSt[i];
    }
    idx += 10;
    encodedSize += 10;
    cout << "Codes :       ";
    for (fileIdx = 0; fileIdx < fileSize; fileIdx++) {
        uint16_t cur = file[fileIdx];
        for (int j = 0; j < 24; j++) {
            uint16_t st = bestDigitsSt[j];
            uint16_t sz = bestDigitsSz[j];
            if (cur == 0) {
                encodeSt(mask[j], st, sz);  //delimiter
                break;
            } else {
                encodeSt((cur-1)%mask[j], st, sz); //next digit
                cur = (cur-1)/mask[j];
            }
        }
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
void BCS(uint8_t dSz[24], uint8_t dSt[24], uint32_t dN)
{
    if (dN >= 3) {
        dSz[dN] = 2;
        dSt[dN] = 2;
        uint64_t fullSz = 0;
        for (int i = 0; i <= maxCode; i++) {
            if (getSum(i, i) != 0) {
                fullSz += getBitCount(i, dSz, dSt)*cnt[i];
            }
        }
        if (fullSz < bestFullBitsSz || bestFullBitsSz == -1)
        {
            bestFullBitsSz = fullSz;
            for (uint32_t i = 0; i < 24; i++)
            {
                bestDigitsSz[i] = dSz[i];
                bestDigitsSt[i] = dSt[i];
            }
        }
        return;
    }
    for(int i=shortestDigitLen;i<=longestDigitLen[dN];i++) {
        for (int j = minSt[dN]; j <= maxSt[dN]; j++) {
            dSz[dN] = i;
            dSt[dN] = j;
            BCS(dSz, dSt, dN + 1);
        }
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
    uint8_t digitLen[24];
    memset(digitLen, 2, sizeof(uint8_t) * 24);
    uint8_t digitSt[24];
    memset(digitSt, 2, sizeof(uint8_t) * 24);
    for (int i = 0; i < 24; i++) {
        bestDigitsSt[i] = 2;
        bestDigitsSz[i] = 2;
    }
    BCS(digitLen, digitSt, 0);

    for (int i = 0; i < 24; i++) {
        mask[i] = (uint32_t)pow(bestDigitsSt[i], bestDigitsSz[i])-1;
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
            uint16_t sz = bestDigitsSz[j];

            uint16_t val = 0;

            for (int k = 0; k < sz; k++) {
                uint16_t f = decode(st);
                val *= st;
                val += f;
            }
            //if (decodedSize < checkSize) cout << "val: " << val << endl;

            //end of number
            if (val == mask[j]) {
                cur += pref[j];
                //if (decodedSize < 10) cout << "Add : " << pref[j] << endl;
                break;
            }
            cur += val*pows[j];
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
        cout << "Falied" << endl;
    } else {
        cout << "Ok" << endl;
    }
}

int main() {

    FILE* in = fopen("resources/data04", "rb");

    fseek(in, 0, SEEK_END);
    fileSize = ftell(in) / sizeof(uint16_t);
    fseek(in, 0, SEEK_SET);
    fread(file, sizeof(uint16_t), fileSize, in);
/*
    if (true){
        fileSize = 10;
        for (int i = 0; i < 10; i++){
            file[i] = i;
        }
    }
*/
    fclose(in);

    preCalc();

    cout << "Digit size : ";
    for (int i = 0; i < 24; i++) {
        cout << (int)bestDigitsSz[i] << " ";
    }
    cout << endl;
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


    /*
    for (int i = 0; i < 10; i++) {
        cout << pows[i] << " " << pref[i] << " " << mask[i] << endl;
    }
    */

    encodeFile();



    cout << "Encoded file: ";
    for (uint32_t i = 10; i < checkSize+10; i++) {
        for (int j = 0; j < 16; j++) {
            cout << ((encoded[i]&(1<<15-j))>>(15-j));
        }
    }
    cout << endl;

    decodeFile();

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
    /*
        if (true){
            fileSize = 10;
            for (int i = 0; i < 10; i++){
                file[i] = i;
            }
        }
    */
    fclose(out);

    return 0;
}


/*
File sizes (in bytes):
Before -> After encoding

13,816,036 -> 8,017,510
1,858 -> 422
20,000 -> 17,200
4,210,664 -> 4,492,434
2,915,710 -> 2,016,200
*/


