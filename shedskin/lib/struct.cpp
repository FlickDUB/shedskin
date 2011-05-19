#include "struct.hpp"
#include <stdio.h>

namespace __struct__ {

__GC_STRING ordering;

char buffy[32];

__ss_int unpack_one(str *s, __ss_int idx, __ss_int count, __ss_int endian) {
    unsigned int r = 0;

    for(int i=0; i<count; i++) {
        unsigned char c = s->__getitem__(i+idx)->unit[0];
        if (endian)
            r += (c << 8*(count-i-1));
        else
            r += (c << 8*i);

    }

    return r;
}


__ss_int unpack_int(char o, char c, int d, str *data, __ss_int *pos) {
    __ss_int result;
    switch(c) {
        case 'H':
            result = unpack_one(data, *pos, 2, o=='>');
             *pos += 2;
            break;
        case 'I':
            result = unpack_one(data, *pos, 4, o=='>');
             *pos += 4;
            break;
        case 'B':
            result = unpack_one(data, *pos, 1, o=='>');
             *pos += 1;
            break;
    }
    return result;
}

str * unpack_str(char o, char c, int d, str *data, __ss_int *pos) {
    str *result = new str(data->unit.substr(*pos, d));
    *pos += d;
    return result;
}
__ss_bool unpack_bool(char o, char c, int d, str *data, __ss_int *pos) {
    return True;
}

double unpack_float(char o, char c, int d, str *data, __ss_int *pos) {
    return 3.141;
}

__ss_int calcsize(str *fmt) {
    __ss_int result = 0;
    str *digits = new str();
    for(unsigned int i=0; i<len(fmt); i++) {
        char c = fmt->unit[i];
        if(ordering.find(c) != -1)
            continue;
        if(::isdigit(c)) {
            digits->unit += c;
        } else {
            int ndigits = 1;
            if(len(digits)) {
                ndigits = __int(digits);
                digits = new str();
            }
            switch(c) {
                case 'H': result += 2*ndigits; break;
                case 'I': result += 4*ndigits; break;
                case 'B': result += ndigits; break;
                case 's': result += ndigits; break;
            }
        }
    }
    return result;
}

int get_itemsize(char order, char c) {
    if(order == '@') {
        switch(c) {
            case 'b': return sizeof(signed char);
            case 'B': return sizeof(unsigned char);
            case 'h': return sizeof(short); /* XXX */
            case 'H': return sizeof(unsigned short);
            case 'i': return sizeof(int);
            case 'I': return sizeof(unsigned int);
            case 'l': return sizeof(long);
            case 'L': return sizeof(unsigned long);
            case 'q': return sizeof(long long);
            case 'Q': return sizeof(unsigned long long);
        }
    } else {
        switch(c) {
            case 'b': return 1;
            case 'B': return 1;
            case 'h': return 2;
            case 'H': return 2;
            case 'i': return 4;
            case 'I': return 4;
            case 'l': return 4;
            case 'L': return 4;
            case 'q': return 8;
            case 'Q': return 8;
        }
    }
}

void fillbuf(char c, __ss_int t, char order, int itemsize) {
    if(order == '@') {
        switch(c) {
            case 'b': *((signed char *)buffy) = t; break;
            case 'B': *((unsigned char *)buffy) = t; break;
            case 'h': *((short *)buffy) = t; break;
            case 'H': *((unsigned short *)buffy) = t; break;
            case 'i': *((int *)buffy) = t; break;
            case 'I': *((unsigned int *)buffy) = t; break;
            case 'l': *((long *)buffy) = t; break;
            case 'L': *((unsigned long *)buffy) = t; break;
            case 'q': *((long long *)buffy) = t; break;
            case 'Q': *((unsigned long long *)buffy) = t; break;
        }
    } else {
        if(order == '>' or order == '!') {
            for(int i=itemsize-1; i>=0; i--) {
                buffy[i] = (unsigned char)(t & 0xff);
                t >>= 8;
            }
        } else {
            for(unsigned int i=0; i<itemsize; i++) {
                buffy[i] = (unsigned char)(t & 0xff);
                t >>= 8;
            }
        }
    }
}

str *pack(int n, str *fmt, ...) {
    pyobj *arg;
    va_list args;
    va_start(args, fmt);
    str *result = new str();
    char order = '@';
    str *digits = new str();
    int itemsize;
    for(unsigned int i=0, j=0; i<n; j++) {
        char c = fmt->unit[j];
        if(ordering.find(c) != -1) {
            order = c;
            continue;
        }
        if(::isdigit(c)) {
            digits->unit += c;
            continue;
        }
        int ndigits = 1;
        if(len(digits)) {
            ndigits = __int(digits);
            digits = new str();
        }
        switch(c) {
            case 'b': 
            case 'B': 
            case 'h': 
            case 'H': 
            case 'i':
            case 'I': 
            case 'l':
            case 'L':
            case 'q': 
            case 'Q':
                itemsize = get_itemsize(order, c);
                for(unsigned int j=0; j<ndigits; j++) {
                    arg = va_arg(args, pyobj *);
                    if(arg->__class__ == cl_int_) {
                        fillbuf(c, ((int_ *)arg)->unit, order, itemsize);
                        for(unsigned int k=0; k<itemsize; k++)
                            result->unit += buffy[k];
                    }
                }
                break;
            case 'c': 
                arg = va_arg(args, pyobj *);
                if(arg->__class__ == cl_str_)
                    result->unit += ((str *)(arg))->unit[0];
                break;
            case 'p': 
                arg = va_arg(args, pyobj *);
                if(arg->__class__ == cl_str_) {
                    int len = ((str *)(arg))->__len__()-1;
                    result->unit += (unsigned char)(len);
                    for(unsigned int j=0; j<len; j++)
                        result->unit += ((str *)(arg))->unit[j];
                }
                break;
            case 's':
                arg = va_arg(args, pyobj *);
                if(arg->__class__ == cl_str_) 
                    result->unit += ((str *)(arg))->unit;
                break;
            case '?':
                for(unsigned int j=0; j<ndigits; j++) {
                    arg = va_arg(args, pyobj *);
                    if(arg->__class__ == cl_bool) {
                        if(((bool_ *)(arg))->unit)
                            result->unit += '\x01';
                        else
                            result->unit += '\x00';
                    }
                }
                break;
        }
        i++;
    }
    va_end(args);
    return result;
}

void __init() {
    ordering = "@<>!=";
}

} // module namespace

