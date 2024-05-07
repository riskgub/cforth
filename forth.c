#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef int64_t cell;
typedef uint64_t ucell;

#define MEMORY_SIZE 65536 * sizeof( cell )
#define STACK_SIZE 256 * sizeof( cell )

#define OP_BRANCH 0x01
#define OP_QBRANCH 0x02
#define OP_CALL 0x03
#define OP_EXIT 0x04
#define OP_EXECUTE 0x05
#define OP_LITERAL 0x06
#define OP_TO_R 0x08
#define OP_R_TO 0x09
#define OP_R_FETCH 0x0A
#define OP_PLUS  0x10
#define OP_MINUS 0x11
#define OP_AND   0x12
#define OP_OR    0x13
#define OP_XOR   0x14
#define OP_LSHIFT 0x15
#define OP_RSHIFT 0x16
#define OP_INVERT 0x17
#define OP_DUP   0x20
#define OP_DROP  0x21
#define OP_SWAP  0x22
#define OP_OVER  0x23
#define OP_CFETCH 0x30
#define OP_FETCH  0x31
#define OP_CSTORE 0x38
#define OP_STORE  0x39
#define OP_LESS_THAN_ZERO 0x40

cell stack[256];
cell rstack[256];
char tib[256];
cell toin;
cell tib_size;

char* memory;
cell* sp;
cell* rp;
char* dp;
char* ip;
cell latest;
cell state;

void push ( cell x )
{
  *(--sp) = x;
}

cell pop ( ) 
{
  return *(sp++);
}

void rpush ( cell x )
{
  *(--rp) = x;
}

cell rpop ( ) 
{
  return *(rp++);
}

void dup ( )
{
  push ( *sp );
}

void drop ( )
{
  sp++;
}

void swap ( )
{
  cell a = pop();
  cell b = pop();
  push( a );
  push( b );
}

void over ( )
{
  push( *(sp+1) ); 
}

void cfetch ( )
{
  push ( *((unsigned char*)pop()) );
}

void fetch ( )
{
  push ( *((cell*)pop()) );
}

void cstore ( )
{
  char* c_addr = (char*)pop();
  unsigned char c = pop();
  *c_addr = c;
}

void store ( )
{
  cell* a_addr = (cell*)pop();
  cell x = pop();
  *a_addr = x;
}

void semicolon ( )
{
  ip = (char*)rpop();
}

void call ( cell addr, int offset )
{
  rpush( (cell)(ip + offset));
  ip = (char*)addr;
  char running = 1;
  printf("ip: %X\n", ip);
  unsigned char op = *(ip++);
  printf("op: %X\n", op);
  while ( running )
  {
    cell a;
    cell b;
    switch ( op )
    {
      case OP_BRANCH: fetch( (cell)ip ); ip = (char*)pop(); break;
      case OP_QBRANCH:
        if ( pop() == 0 )
        {
          fetch( (cell)ip );
          ip = (char*)pop();
        }
        else 
          ip += sizeof( cell );
      case OP_CALL:  fetch( ip ); call( pop(), sizeof( cell ) ); break;
      case OP_EXIT:  semicolon(); running = 0; break;
      case OP_LITERAL:  fetch( (cell)ip ); ip += sizeof( cell ); break;
      case OP_EXECUTE: call( pop(), 0 ); break;
      case OP_TO_R:  rpush( pop() ); break;
      case OP_R_TO:  push( rpop() ); break;
      case OP_R_FETCH:  push( *rp ); break;
      case OP_PLUS: push( pop() + pop() ); break;
      case OP_MINUS:
        b = pop();
        a = pop();
        push( a - b );
        break;
      case OP_AND: push( pop() & pop() ); break;
      case OP_OR:  push( pop() | pop() ); break;
      case OP_XOR: push( pop() ^ pop() ); break;
      case OP_LSHIFT:
        b = pop();
        a = pop();
        push( a << b );
        break;
      case OP_RSHIFT:
        b = pop();
        a = pop();
        push( a >> b );
        break;
      case OP_INVERT: push( ~pop() ); break;
      case OP_DUP: dup(); break;
      case OP_DROP: drop(); break;
      case OP_SWAP: swap(); break;
      case OP_OVER: over(); break;
      case OP_CFETCH:  cfetch( pop() ); break;
      case OP_FETCH:   fetch( pop() ); break;
      case OP_CSTORE:
        b = pop();
        a = pop();
        cstore( a, b );
        break;
      case OP_STORE:
        b = pop();
        a = pop();
        store( a, b );
        break;
      case OP_LESS_THAN_ZERO: push( pop() < 0 ? -1 : 0 ); break;
      default: semicolon(); running = 0;
    }
  }



}


void execute ( )
{
  call( pop(), 0 );
}

void one_plus ( )
{
  (*sp)++;
}

void one_minus ( )
{
  (*sp)--;
}

void allot ( )
{
  cell n = pop();
  dp = dp + n;
}

void c_comma ( ) 
{
  *(dp++) = pop();
}

void comma ( )
{
  *((cell*)dp++) = pop();
  push( sizeof(cell) );
  allot();
}

void count ( )
{
  dup();
  one_plus();
  swap();
  cfetch();
}

void string_equal ( )
{
  cell u2 = pop();
  char* c_addr2 = (char*)pop();
  cell u1 = pop();
  char* c_addr1 = (char*)pop();
  if ( u1 != u2 )
  {
    push( 0 );
    return;
  }
  while ( u1 )
  {
    unsigned char char1 = *c_addr1++;
    unsigned char char2 = *c_addr2++;
    if ( char1 != char2 )
    {
      push( 0 );
      return;
    }
    u1--;
  }
  push( -1 );
}

void string_comma ( )
{
  dup(); 
  c_comma();
  ucell len = pop();
  for ( ucell i = 0; i < len; i++ )
  {
    dup();
    cfetch();
    c_comma();
    one_plus();
  }
  drop();
}

void compile_comma ( )
{
  push( OP_CALL );
  c_comma();
  comma( pop );
}

void header_comma ( )
{
  cell old_dp = (cell)dp;
  push( latest );
  comma();
  latest = old_dp;
  push( 0x80 );
  c_comma();
  string_comma();
}

char* link_to_xt ( cell* link )
{
  char* c_addr = (char*)(link+1)+2;
  unsigned char len = *c_addr;
  printf("len: %X\n", len);
  return c_addr + len + 1;
}

void sfind ( )
{
  ucell u = pop();
  char* c_addr1 = (char*)pop();
  cell* word = (cell*)latest; 
  while ( word )
  {
    unsigned char flags = *((char*)(word+1)+1);
    push( (cell)(word+1)+2 );
    count();
    push( (cell)c_addr1 );
    push( u );
    string_equal();
    cell f = pop();
    if ( f && (flags & 0x80) )
    {
      push( (cell)link_to_xt( word ) );
      push( flags & 0x40 ? 1 : -1 );
      return;
    }
    word = (cell*)(*word);
  }
  push( 0 );
}

void skip_whitespace ( )
{
  while ( tib[toin] <= 32 && toin < tib_size )
    toin++;
}

void parse ( )
{
  skip_whitespace();
  ucell start = toin;
  char delim = pop();
  while ( tib[toin] != delim && toin < tib_size )
    toin++;
  ucell u = toin - start;
  push ( (cell)(&tib[start]) );
  push ( u );
}

void parse_name ( )
{
  skip_whitespace();
  ucell start = toin;
  while ( tib[toin] > ' ' && toin < tib_size )
    toin++;
  ucell u = toin - start;
  push ( (cell)(&tib[start]) );
  push ( u );
}

void accept ( )
{
  ucell l = 0;
  char c = '\0';
  ucell u = pop();
  char* c_addr = (char*)pop();
  while ( c != '\n' && l < u )
  {
    c = getchar();
    if ( c >= ' ' )
      c_addr[l] = c;
    l++;
  }
  l--;
  push(l);
}

void bye ( )
{
  exit( 1 );
}

void refill ( )
{
  toin = 0;
  push( (cell)(&tib) );
  push( 256 );
  accept();
  tib_size = pop();
  push( -1 );
}

void type ( )
{
  ucell u = pop();
  char* c_addr = (char*)pop();
  while ( u-- )
    putchar(*(c_addr++));
}

void literal ( cell x )
{
  push( OP_LITERAL );
  c_comma();
  push( x );
  comma();
}

#define HEADER(c,u) push((cell)c);push(u);header_comma();
#define EXIT push(OP_EXIT);c_comma();

void init_dictionary ( )
{
  latest = 0;
  HEADER("DUP",3)
  push( OP_DUP );
  c_comma();
  EXIT

  HEADER("123",3)
  literal(123);
  EXIT

  HEADER("[",1)
  literal( 0 );
  literal( (cell)state );
  push( OP_STORE );
  c_comma();
  EXIT
}

int main ( )
{
  memory = malloc( MEMORY_SIZE );
  sp = &stack[255];
  rp = &rstack[255];
  dp = memory;
  init_dictionary();
  while ( 1 )
  {
    refill();
    drop();
    parse_name();
    sfind();
    cell flag = pop();
    cell xt = pop();
    if ( flag != 0 )
    {
      push( xt );
      if ( flag == 1 || ( flag == -1 && state == 0 )  )
      {
        execute();
      }
      else 
      {
        if ( state )
          compile_comma();
      }
      puts("ok.");
    }
    else 
    {
      puts("Undefined word");
    }
  }
  return 1;
}
