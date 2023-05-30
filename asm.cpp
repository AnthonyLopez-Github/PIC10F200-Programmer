#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <sstream>
#include <vector>

enum OP {
    // byte-oriented
    ADDWF = 0,
    ANDWF,
    CLRF,
    CLRW,
    COMF,
    DECF,
    DECFSZ,
    INCF,
    INCFSZ,
    IORWF,
    MOVF,
    MOVWF,
    NOP,
    RLF,
    RRF,
    SUBWF,
    SWAPF,
    XORWF,

    // bit-oriented
    BCF,
    BSF,
    BTFSC,
    BTFSS,

    // literal/control
    ANDLW,
    CALL,
    CLRWDT,
    GOTO,
    IORLW,
    MOVLW,
    OPTION,
    RETLW,
    SLEEP,
    TRIS,
    XORLW,
};

const char * OP_STRINGS[] = {
    "ADDWF",  "ANDWF",  "CLRF",  "CLRW",   "COMF",  "DECF",  "DECFSZ",
    "INCF",   "INCFSZ", "IORWF", "MOVF",   "MOVWF", "NOP",   "RLF",
    "RRF",    "SUBWF",  "SWAPF", "XORWF",  "BCF",   "BSF",   "BTFSC",
    "BTFSS",  "ANDLW",  "CALL",  "CLRWDT", "GOTO",  "IORLW", "MOVLW",
    "OPTION", "RETLW",  "SLEEP", "TRIS",   "XORLW",

};

struct Instr {
    OP op; // nya

    int f; // register file
    int d; // destination bit (0 - store in W, 1 - store in reg)
    int k; // literal
    int b; // bit
};

struct ParseOutput {
    std::vector< Instr > instructions;
    std::vector< int > lineNumbers;

    std::vector< const char * > labels;
    std::vector< int > labelTargets;
};

std::string toUpper( std::string str )
{
    std::stringstream ss;
    for ( char c : str ) {
        ss << (char) ( std::toupper( c ) );
    }

    return ss.str();
}

bool parseHex( const char * str, int & val )
{
    int len = std::strlen( str );

    if ( len < 3 )
        return false;
    if ( str[ 0 ] != '0' )
        return false;
    if ( str[ 1 ] != 'x' )
        return false;

    int x = 0;
    int factor = 1;
    for ( int i = len - 1; i >= 2; i-- ) {
        int v;
        switch ( str[ i ] ) {
        case '0':
            v = 0;
            break;
        case '1':
            v = 1;
            break;
        case '2':
            v = 2;
            break;
        case '3':
            v = 3;
            break;
        case '4':
            v = 4;
            break;
        case '5':
            v = 5;
            break;
        case '6':
            v = 6;
            break;
        case '7':
            v = 7;
            break;
        case '8':
            v = 8;
            break;
        case '9':
            v = 9;
            break;
        case 'a':
            v = 10;
            break;
        case 'b':
            v = 11;
            break;
        case 'c':
            v = 12;
            break;
        case 'd':
            v = 13;
            break;
        case 'e':
            v = 14;
            break;
        case 'f':
            v = 15;
            break;
        default:
            return false;
        }
        x += v * factor;
        factor *= 16;
    }

    val = x;

    return true;
}

void parse( std::span< char > content, ParseOutput & out )
{
    std::vector< char * > tokens;
    std::vector< int > lineNumbers;

    // parse tokens
    int currentLine = 1;
    for ( int i = 0; content[ i ] != '\0'; i++ ) {
        bool wasBlank = i == 0 ? true : content[ i - 1 ] == '\0';

        if ( content[ i ] == ';' ) {
            // a line comment
            for ( ; content[ i ] != '\0'; i++ ) {
                if ( content[ i ] == '\n' ) {
                    break;
                }
                content[ i ] = '\0';
            }

            if ( content[ i ] == '\0' ) {
                break;
            }

            wasBlank = true;
        }

        if ( content[ i ] == '\n' ) {
            currentLine++;
        }

        if ( std::isspace( content[ i ] ) ) {
            content[ i ] = '\0';
        } else {
            if ( wasBlank ) {
                tokens.push_back( &content[ i ] );
                lineNumbers.push_back( currentLine );
            }
        }
    }

    // for ( int i = 0; i < std::ssize( tokens ); i++ ) {
    //     std::cout << lineNumbers[ i ] << ": " << tokens[ i ] << std::endl;
    // }

    auto expectNum = [ & ]( int cursor, int & num ) {
        if ( cursor >= std::ssize( tokens ) ) {
            std::cerr << "ERROR [line " << lineNumbers[ cursor ] << "]: ";
            std::cerr << "Expected number, got EOF" << std::endl;
            exit( 1 );
        }

        const char * token = tokens[ cursor ];

        // parse hex characters
        if ( !parseHex( token, num ) ) {
            std::cerr << "ERROR [line " << lineNumbers[ cursor ] << "]: ";
            std::cerr << "Expected number, got: " << token << std::endl;
            exit( 1 );
        }
    };

    // parse instructions
    for ( int i = 0; i < std::ssize( tokens ); i++ ) {
        char * token = tokens[ i ];
        int tokenLen = std::strlen( token );
        int lineNumber = lineNumbers[ i ];

        // check if its a label
        if ( token[ tokenLen - 1 ] == ':' ) {
            // erase ':'
            token[ tokenLen - 1 ] = '\0';
            out.labels.push_back( token );
            out.labelTargets.push_back( std::ssize( out.instructions ) );
            continue;
        }

        // find in op list
        int opIndex;
        int opStringsLen = std::ssize( OP_STRINGS );
        for ( opIndex = 0; opIndex < opStringsLen; opIndex++ ) {
            if ( toUpper( token ) == OP_STRINGS[ opIndex ] ) {
                break;
            }
        }

        if ( opIndex >= opStringsLen ) {
            std::cerr << "ERROR [line " << lineNumber << "]: ";
            std::cerr << " Instruction not implemented: " << toUpper( token )
                      << std::endl;
            exit( 1 );
        }

        // parse args
        Instr instr;
        instr.op = (OP) opIndex;

        switch ( instr.op ) {
        case ADDWF: // byte-oriented ops
        case ANDWF:
        case COMF:
        case DECF:
        case DECFSZ:
        case INCF:
        case INCFSZ:
        case IORWF:
        case MOVF:
        case RLF:
        case RRF:
        case SUBWF:
        case SWAPF:
        case XORWF:
            expectNum( ++i, instr.f );
            expectNum( ++i, instr.d );
            break;
        case CLRF:
        case MOVWF:
            expectNum( ++i, instr.f );
            break;
        case CLRW:
        case NOP:
            break;
        case BCF: // bit-oriented ops
        case BSF:
        case BTFSC:
        case BTFSS:
            expectNum( ++i, instr.f );
            expectNum( ++i, instr.b );
            break;
        case ANDLW: // literal/control ops
        case CALL:
        case GOTO:
        case IORLW:
        case MOVLW:
        case RETLW:
        case XORLW:
            expectNum( ++i, instr.k );
            break;
        case TRIS:
            expectNum( ++i, instr.f );
            break;
        case CLRWDT:
        case OPTION:
        case SLEEP:
            break;
        }

        out.instructions.push_back( instr );
        out.lineNumbers.push_back( lineNumber );
    }
}

void write( const ParseOutput & in, std::vector< uint8_t > & out )
{
    out.push_back( 0 );
    int bitCount = 0;

    int lineNumber = -1;

    auto append = [ & ]( int val, int bits ) {
        int mask = ( 0x1 << bits ) - 1;
        if ( ( val & mask ) != val ) {
            std::cerr << "WARNING [line " << lineNumber << "]: ";
            std::cerr << " Value will be truncated to " << bits
                      << " bit(s) in width: " << val << std::endl;
            // exit( 1 );
        }

        for ( int i = bits - 1; i >= 0; i-- ) {
            uint8_t & outByte = out.back();

            uint8_t bit = ( val >> i ) & 0x1;

            outByte |= bit << ( 7 - bitCount );
            bitCount++;

            if ( bitCount >= 8 ) {
                out.push_back( 0 );
                bitCount = 0;
            }
        }
    };

    for ( int i = 0; i < std::ssize( in.instructions ); i++ ) {
        const Instr & instr = in.instructions[ i ];

        lineNumber = in.lineNumbers[ i ];

        switch ( instr.op ) {
        case ADDWF:
            append( 0b0001'11, 6 );
            append( instr.d, 1 );
            append( instr.f, 5 );
            break;
        case ANDWF:
            append( 0b0001'01, 6 );
            append( instr.d, 1 );
            append( instr.f, 5 );
            break;
        case CLRF:
            append( 0b0000'011, 7 );
            append( instr.f, 5 );
            break;
        case CLRW:
            append( 0b0000'0100'0000, 12 );
            break;
        case COMF:
            append( 0b0010'01, 6 );
            append( instr.d, 1 );
            append( instr.f, 5 );
            break;
        case DECF:
            append( 0b0000'11, 6 );
            append( instr.d, 1 );
            append( instr.f, 5 );
            break;
        case DECFSZ:
            append( 0b0010'11, 6 );
            append( instr.d, 1 );
            append( instr.f, 5 );
            break;
        case INCF:
            append( 0b0010'10, 6 );
            append( instr.d, 1 );
            append( instr.f, 5 );
            break;
        case INCFSZ:
            append( 0b0011'11, 6 );
            append( instr.d, 1 );
            append( instr.f, 5 );
            break;
        case IORWF:
            append( 0b0001'00, 6 );
            append( instr.d, 1 );
            append( instr.f, 5 );
            break;
        case MOVF:
            append( 0b0010'00, 6 );
            append( instr.d, 1 );
            append( instr.f, 5 );
            break;
        case MOVWF:
            append( 0b0000'001, 7 );
            append( instr.f, 5 );
            break;
        case NOP:
            append( 0b0000'0000'00, 12 );
            break;
        case RLF:
            append( 0b0011'01, 6 );
            append( instr.d, 1 );
            append( instr.f, 5 );
            break;
        case RRF:
            append( 0b0011'00, 6 );
            append( instr.d, 1 );
            append( instr.f, 5 );
            break;
        case SUBWF:
            append( 0b0000'10, 6 );
            append( instr.d, 1 );
            append( instr.f, 5 );
            break;
        case SWAPF:
            append( 0b0011'10, 6 );
            append( instr.d, 1 );
            append( instr.f, 5 );
            break;
        case XORWF:
            append( 0b0001'10, 6 );
            append( instr.d, 1 );
            append( instr.f, 5 );
            break;
        case BCF:
            append( 0b0100, 4 );
            append( instr.b, 3 );
            append( instr.f, 5 );
            break;
        case BSF:
            append( 0b0101, 4 );
            append( instr.b, 3 );
            append( instr.f, 5 );
            break;
        case BTFSC:
            append( 0b0110, 4 );
            append( instr.b, 3 );
            append( instr.f, 5 );
            break;
        case BTFSS:
            append( 0b0111, 4 );
            append( instr.b, 3 );
            append( instr.f, 5 );
            break;
        case ANDLW:
            append( 0b1110, 4 );
            append( instr.k, 8 );
            break;
        case CALL:
            append( 0b1001, 4 );
            append( instr.k, 8 );
            break;
        case CLRWDT:
            append( 0b0000'0000'0100, 12 );
            break;
        case GOTO:
            append( 0b101, 3 );
            append( instr.k, 9 );
            break;
        case IORLW:
            append( 0b1101, 4 );
            append( instr.k, 8 );
            break;
        case MOVLW:
            append( 0b1100, 4 );
            append( instr.k, 8 );
            break;
        case OPTION:
            append( 0b0000'0000'0010, 12 );
            break;
        case RETLW:
            append( 0b1000, 4 );
            append( instr.k, 8 );
            break;
        case SLEEP:
            append( 0b0000'0000'0011, 12 );
            break;
        case TRIS:
            append( 0b0000'0000'0, 9 );
            append( instr.f, 3 );
            break;
        case XORLW:
            append( 0b1111, 4 );
            append( instr.k, 8 );
            break;
        }
    }

    std::cout << "\n*** BINARY ***\n\n";

    for ( uint8_t x : out ) {
        for ( int i = 0; i < 4; i++ ) {
            bool v = ( x >> ( 7 - i ) ) & 0x1;
            std::cout << ( v ? '1' : '0' );
        }

        std::cout << ' ';

        for ( int i = 0; i < 4; i++ ) {
            bool v = ( x >> ( 3 - i ) ) & 0x1;
            std::cout << ( v ? '1' : '0' );
        }

        std::cout << ' ';
    }

    std::cout << std::endl;
}

int main( int argc, char ** argv )
{
    if ( argc < 2 ) {
        std::cout << argv[ 0 ] << " <filename>" << std::endl;
        return 1;
    }

    std::string path = argv[ 1 ];
    std::ifstream file( path );

    if ( !file ) {
        std::cerr << "File not found: " << path << std::endl;
        return 1;
    }

    std::string source(
        ( std::istreambuf_iterator< char >( file ) ),
        ( std::istreambuf_iterator< char >() )
    );

    std::vector< char > content;
    for ( char c : source ) {
        content.push_back( c );
    }
    content.push_back( '\0' );

    ParseOutput parseOutput;
    parse( content, parseOutput );
    std::vector< uint8_t > out;
    write( parseOutput, out );

    std::filesystem::path filePath = path;

    std::string base = filePath.stem().string();
    std::ofstream outFile( base + ".bin", std::ios::out | std::ios::binary );

    outFile.write( (char *) out.data(), out.size() );

    // std::cout << "success" << std::endl;

    return 0;
}
