#include "gtest/gtest.h"

#include <algorithm>

extern "C" {

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include "ring-buffer.h"

}

using namespace std;

class RingBufferTest : public ::testing::Test {

public:

	static const unsigned max_buf_len = 16;
	static const uint8_t buf_template[ max_buf_len ];
	static const unsigned n = 4;
	static uint8_t storage[ n * max_buf_len ];
	static const unsigned cap[ n ];
	//bool expected_valid[ n ];
	ring_buffer_t rb[ n ];
	static const uint8_t buff_fill_base = 1;
	uint8_t *buf[ n ];

	RingBufferTest();

	virtual ~RingBufferTest() {
	}

	// called right after constructor
	virtual void SetUp();

	// called right before deconstructor
	virtual void TearDown();
};

const uint8_t RingBufferTest::buf_template[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, };
const unsigned RingBufferTest::cap[] = {  0, 1, 2, 11, };
uint8_t RingBufferTest::storage[];

RingBufferTest::RingBufferTest() {
}

void RingBufferTest::SetUp() {
	static const int expected_r = EXIT_SUCCESS;
	int actual_r;
	int i;
	for( i = 0; i < n; i++ ) {
		buf[ i ] = & storage[ i * max_buf_len ];
		actual_r = ring_buffer_init( &rb[ i ], cap[ i ], buf[ i ] );
		ASSERT_EQ( expected_r, actual_r );
		memcpy( buf[ i ], RingBufferTest::buf_template, cap[ i ] );
	}
}

void RingBufferTest::TearDown() {
}

static void ring_buffer_valid_after_init( ring_buffer_t *rb, unsigned expected_capacity ) {

	static const unsigned expected_head = 0;
	static const unsigned expected_len = 0;

	ASSERT_NE( (void *)NULL, rb );

	EXPECT_EQ( expected_capacity, rb->capacity );
	EXPECT_EQ( expected_head, rb->head );
	EXPECT_EQ( expected_len, rb->len );

	ASSERT_NE( (void *)NULL, rb->peak );
	ASSERT_NE( (void *)NULL, rb->read );
	ASSERT_NE( (void *)NULL, rb->write );
	ASSERT_NE( (void *)NULL, rb->skip );
	ASSERT_NE( (void *)NULL, rb->size );
	ASSERT_NE( (void *)NULL, rb->available );
	ASSERT_NE( (void *)NULL, rb->reset );
	ASSERT_NE( (void *)NULL, rb->buffer );
}

TEST_F( RingBufferTest, ContigDeclTest ) {

	int expected_r;
	int actual_r;

	ring_buffer_t *foo;
	const unsigned expected_capacity = 1;

	// declares 'uint8_t foo_buffer[];'
	RING_BUFFER_DECL_CONTIG( expected_capacity, foo );
	foo = RING_BUFFER_CONTIG_HANDLE( foo );

	ASSERT_NE( (void *)NULL, foo );

	expected_r = EXIT_SUCCESS;
	actual_r = ring_buffer_init( foo, expected_capacity, foo_buffer );

	ASSERT_EQ( expected_r, actual_r );

	ring_buffer_valid_after_init( foo, expected_capacity );
}

TEST_F( RingBufferTest, RingBufferInitTest ) {
	int i;
//	int expected_r;
	static const int expected_r = EXIT_SUCCESS;
	int actual_r;
	for( i = 0; i < n; i++ ) {
//		expected_r = false == expected_valid[ i ] ? -EINVAL : EXIT_SUCCESS;
		actual_r = ring_buffer_init( &rb[ i ], cap[ i ], buf[ i ] );
		EXPECT_EQ( expected_r, actual_r );
//		if ( expected_valid[ i ] ) {
			ring_buffer_valid_after_init( & rb[ i ], cap[ i ] );
//		}
	}
}

// XXX: @CF: FIXME: probably can do less here, since there is partial duplication of the skip tests
static void _do_read_test( bool peak, ring_buffer_t *rb, int expected_r ) {

	const uint8_t *expected_val = RingBufferTest::buf_template;

	int i;
	int actual_r;

	unsigned expected_head;
	unsigned actual_head;
	unsigned original_head;

	unsigned expected_len;
	unsigned actual_len;
	uint8_t actual_val[ RingBufferTest::max_buf_len ];
	unsigned actual_val_len;

	original_head = rb->head;

	actual_val_len = 0 == rb->len ? 1 : expected_r;
	if ( peak || 0 == expected_r ) {
		// for peak operations, the length and head MUST NOT CHANGE!!
		expected_len = rb->len;
		expected_head = rb->head;
	} else {
		// for read operations where >= 1 byte is read, the length and head MUST CHANGE!!
		expected_len = (int)rb->len - expected_r;
		expected_head = rb->head + expected_r;
		if ( 0 != rb->capacity ) {
			 expected_head %= rb->capacity;
		}
	}

	if ( peak ) {
		actual_r = rb->peak( rb, actual_val, actual_val_len );
	} else {
		actual_r = rb->read( rb, actual_val, actual_val_len );
	}
	actual_len = rb->len;
	actual_head = rb->head;

	EXPECT_EQ( expected_r, actual_r );
	EXPECT_EQ( expected_len, actual_len );
	EXPECT_EQ( expected_head, actual_head );
	if ( actual_r > 0 ) {
		// verify data
		for( i = 0; i < actual_r; i++ ) {
			EXPECT_EQ( expected_val[ ( original_head + i ) % rb->capacity ], actual_val[ i ] );
		}
	}

}
static void do_peak_test( ring_buffer_t *rb, int expected_r ) {
	_do_read_test( true, rb, expected_r );
}
static void do_read_test( ring_buffer_t *rb, int expected_r ) {
	_do_read_test( false, rb, expected_r );
}

//
// Tests for ring_buffer_t::peak()
//

// try to peak at 0 elements
TEST_F( RingBufferTest, RingBufferPeakTestEmpty ) {
	static const int expected_r = 0;
	do_peak_test( &rb[ 0 ], expected_r );
}

// peak the 0th element
TEST_F( RingBufferTest, RingBufferPeakTestOne ) {
	int i;
	int expected_r;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = rb[ i ].capacity;
		expected_r = std::min( 1, (int) rb[ i ].len );
		do_peak_test( &rb[ i ], expected_r );
	}
}

// peak at all of the elements starting from the 0th
TEST_F( RingBufferTest, RingBufferPeakTestAll ) {
	int i;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = rb[ i ].capacity;
		int expected_r = rb[ i ].capacity;
		do_peak_test( &rb[ i ], expected_r );
	}
}

// peak at all elements (head of buffer is in the middle rather than at 0)
TEST_F( RingBufferTest, RingBufferPeakTestWrap ) {
	int i;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = rb[ i ].capacity;
		rb[ i ].head = rb[ i ].len / 2;
		int expected_r = rb[ i ].capacity;
		do_peak_test( &rb[ i ], expected_r );
	}
}

// request to peak at len <= n <= cap elements
TEST_F( RingBufferTest, RingBufferPeakTestMoreThanLen ) {
	int expected_r;
	int actual_r;
	uint8_t actual_val[ max_buf_len ];
	int i;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = rb[ i ].capacity / 2;
		expected_r = rb[ i ].len;
		actual_r = rb[ i ].peak( &rb[ i ], actual_val, expected_r + 1 );
		EXPECT_EQ( expected_r, actual_r );
	}
}

// request to peak at len <= cap <= n elements
TEST_F( RingBufferTest, RingBufferPeakTestMoreThanCap ) {
	int expected_r;
	int actual_r;
	uint8_t actual_val[ max_buf_len ];
	int i;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = rb[ i ].capacity;
		expected_r = rb[ i ].len;
		actual_r = rb[ i ].peak( &rb[ i ], actual_val, expected_r + 1 );
		EXPECT_EQ( expected_r, actual_r );
	}
}

//
// Tests for ring_buffer_t::read()
//

// try to read 0 elements
TEST_F( RingBufferTest, RingBufferReadTestEmpty ) {
	static const int expected_r = 0;
	do_read_test( &rb[ 0 ], expected_r );
}

// read the 0th element
TEST_F( RingBufferTest, RingBufferReadTestOne ) {
	int i;
	int expected_r;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = rb[ i ].capacity;
		expected_r = std::min( 1, (int) rb[ i ].len );
		do_read_test( &rb[ i ], expected_r );
	}
}

// read at all of the elements starting from the 0th
TEST_F( RingBufferTest, RingBufferReadTestAll ) {
	int i;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = rb[ i ].capacity;
		int expected_r = rb[ i ].capacity;
		do_read_test( &rb[ i ], expected_r );
	}
}

// read at all elements (head of buffer is in the middle rather than at 0)
TEST_F( RingBufferTest, RingBufferReadTestWrap ) {
	int i;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = rb[ i ].capacity;
		rb[ i ].head = rb[ i ].len / 2;
		int expected_r = rb[ i ].capacity;
		do_read_test( &rb[ i ], expected_r );
	}
}

// request to read at len <= n <= cap elements
TEST_F( RingBufferTest, RingBufferReadTestMoreThanLen ) {
	int expected_r;
	int actual_r;
	uint8_t actual_val[ max_buf_len ];
	int i;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = rb[ i ].capacity / 2;
		expected_r = rb[ i ].len;
		actual_r = rb[ i ].read( &rb[ i ], actual_val, expected_r + 1 );
		EXPECT_EQ( expected_r, actual_r );
	}
}

// request to read at len <= cap <= n elements
TEST_F( RingBufferTest, RingBufferReadTestMoreThanCap ) {
	int expected_r;
	int actual_r;
	uint8_t actual_val[ max_buf_len ];
	int i;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = rb[ i ].capacity;
		expected_r = rb[ i ].len;
		actual_r = rb[ i ].read( &rb[ i ], actual_val, expected_r + 1 );
		EXPECT_EQ( expected_r, actual_r );
	}
}

//
// Tests for ring_buffer_t::write()
//

static void do_write_test( ring_buffer_t *rb, int expected_r ) {
	// all zeros
	static const uint8_t expected_val[ RingBufferTest::max_buf_len ] = {};

	int i;
	int actual_r;

	unsigned expected_head;
	unsigned actual_head;
	unsigned original_head;

	unsigned expected_len;
	unsigned actual_len;
	uint8_t *actual_val = (uint8_t *)rb->buffer;
	unsigned expected_val_len;

	original_head = rb->head;
	expected_head = rb->head;

	expected_val_len = std::min( (unsigned) expected_r, rb->available( rb ) );
	expected_val_len = 0 == expected_val_len ? 1 : expected_val_len;
	expected_len = (int)rb->len + expected_r;

	actual_r = rb->write( rb, (void *)expected_val, expected_val_len );

	actual_len = rb->len;
	actual_head = rb->head;

	EXPECT_EQ( expected_r, actual_r );
	EXPECT_EQ( expected_len, actual_len );
	EXPECT_EQ( expected_head, actual_head );
	if ( actual_r > 0 ) {
		// verify data
		for( i = 0; i < actual_r; i++ ) {
			EXPECT_EQ( expected_val[ i ], actual_val[ ( original_head  + i ) % rb->capacity ] );
		}
	}
}

// try to write 0 elements
TEST_F( RingBufferTest, RingBufferWriteTestEmpty ) {
	static const int expected_r = 0;
	do_write_test( &rb[ 0 ], expected_r );
}

// write the 0th element
TEST_F( RingBufferTest, RingBufferWriteTestOne ) {
	int i;
	int expected_r;
	unsigned avail;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = 0;
		avail = rb[ i ].available( & rb[ i ] );
		expected_r = std::min( 1, (int)avail );
		do_write_test( &rb[ i ], expected_r );
	}
}

// write all of the elements starting from the 0th
TEST_F( RingBufferTest, RingBufferWriteTestAll ) {
	int i;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = 0;
		int expected_r = rb[ i ].capacity;
		do_write_test( &rb[ i ], expected_r );
	}
}

// write all elements (head of buffer is in the middle rather than at 0)
TEST_F( RingBufferTest, RingBufferWriteTestWrap ) {
	int i;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = 0;
		rb[ i ].head = rb[ i ].capacity / 2;
		int expected_r = rb[ i ].capacity;
		do_write_test( &rb[ i ], expected_r );
	}
}

// write len <= n <= cap elements
TEST_F( RingBufferTest, RingBufferWriteTestMoreThanAvail ) {
	int expected_r;
	int actual_r;
	uint8_t actual_val[ max_buf_len ];
	int i;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = rb[ i ].capacity / 2;
		expected_r = rb[ i ].available( & rb[ i ] );
		actual_r = rb[ i ].write( &rb[ i ], actual_val, expected_r + 1 );
		EXPECT_EQ( expected_r, actual_r );
	}
}

// write len <= cap <= n elements
TEST_F( RingBufferTest, RingBufferWriteTestMoreThanCap ) {
	int expected_r;
	int actual_r;
	uint8_t actual_val[ max_buf_len ];
	int i;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = rb[ i ].capacity / 2;
		expected_r = rb[ i ].available( & rb[ i ] );
		actual_r = rb[ i ].write( &rb[ i ], actual_val, rb[ i ].capacity + 1 );
		EXPECT_EQ( expected_r, actual_r );
	}
}

//
// Tests for ring_buffer_t::skip()
//

static void do_skip_test( ring_buffer_t *rb, unsigned requested_skip ) {
	int expected_r;
	int actual_r;
	int expected_head;
	int actual_head;
	int expected_len;
	int actual_len;

	expected_r = std::min( requested_skip, rb->len );
	expected_head = rb->head + expected_r;
	if ( rb->capacity > 0 ) {
		expected_head %= rb->capacity;
	}
	expected_len = rb->len - expected_r;

	actual_r = rb->skip( rb, requested_skip );
	actual_head = rb->head;
	actual_len = rb->len;

	EXPECT_EQ( expected_r, actual_r );
	EXPECT_EQ( expected_head, actual_head );
	EXPECT_EQ( expected_len, actual_len );
}

// skip zero elements
TEST_F( RingBufferTest, RingBufferSkipZero ) {
	static const int requested_skip = 0;
	int i;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = 0;
		do_skip_test( &rb[ i ], requested_skip );
	}
}

// skip one element
TEST_F( RingBufferTest, RingBufferSkipOne ) {
	static const int requested_skip = 1;
	int i;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = cap[ i ];
		do_skip_test( &rb[ i ], requested_skip );
	}
}

// skip all elements starting from the 0th
TEST_F( RingBufferTest, RingBufferSkipAll ) {
	int requested_skip;
	int i;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = cap[ i ];
		requested_skip = rb[ i ].len;
		do_skip_test( &rb[ i ], requested_skip );
	}
}

// skip at all elements (head of buffer is in the middle rather than at 0)
TEST_F( RingBufferTest, RingBufferSkipWrap ) {
	int requested_skip;
	int i;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = cap[ i ];
		rb[ i ].head = rb[ i ].len / 2;
		requested_skip = rb[ i ].len;
		do_skip_test( &rb[ i ], requested_skip );
	}
}

// skip len <= n <= cap elements
TEST_F( RingBufferTest, RingBufferSkipTestMoreThanLen ) {
	int requested_skip;
	int i;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = cap[ i ] / 2;
		requested_skip = rb[ i ].len + 1;
		do_skip_test( &rb[ i ], requested_skip );
	}
}

// skip len <= cap <= n elements
TEST_F( RingBufferTest, RingBufferSkipTestMoreThanCap ) {
	int requested_skip;
	int i;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = cap[ i ] / 2;
		requested_skip = rb[ i ].capacity + 1;
		do_skip_test( &rb[ i ], requested_skip );
	}
}

//
// Tests for ring_buffer_t::size()
//

static void do_size_test( ring_buffer_t *rb, unsigned expected_size ) {
	unsigned actual_size;

	actual_size = rb->size( rb );

	EXPECT_EQ( expected_size, actual_size );
}

// size zero elements
TEST_F( RingBufferTest, RingBufferSizeZero ) {
	static const unsigned expected_size = 0;
	int i;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = expected_size;
		do_size_test( &rb[ i ], expected_size );
	}
}

// size one element
TEST_F( RingBufferTest, RingBufferSizeOne ) {
	static const unsigned expected_size = 1;
	int i;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = expected_size;
		do_size_test( &rb[ i ], expected_size );
	}
}

// size of all elements starting from the 0th
TEST_F( RingBufferTest, RingBufferSizeAll ) {
	unsigned expected_size;
	int i;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = rb[ i ].capacity;
		expected_size = rb[ i ].len;
		do_size_test( &rb[ i ], expected_size );
	}
}

// size of all elements (head of buffer is in the middle rather than at 0)
TEST_F( RingBufferTest, RingBufferSizeWrap ) {
	unsigned expected_size;
	int i;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = rb[ i ].capacity;
		rb[ i ].head = rb[ i ].len / 2;
		expected_size = rb[ i ].len;
		do_size_test( &rb[ i ], expected_size );
	}
}

//
// Tests for ring_buffer_t::available()
//

static void do_available_test( ring_buffer_t *rb, unsigned size ) {
	unsigned actual_available;
	unsigned expected_available;

	expected_available = rb->capacity - size;

	actual_available = rb->available( rb );

	EXPECT_EQ( expected_available, actual_available );
}

// avaiability when size is zero elements
TEST_F( RingBufferTest, RingBufferAvailableZero ) {
	static const unsigned size = 0;
	int i;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = size;
		do_available_test( &rb[ i ], size );
	}
}

// avaiability when size is one element
TEST_F( RingBufferTest, RingBufferAvailableOne ) {
	unsigned size;
	int i;
	for( i = 0; i < n; i++ ) {
		size = std::min( 1, (int) rb[ i ].capacity );
		rb[ i ].len = size;
		do_available_test( &rb[ i ], size );
	}
}

// avaiability when size is all elements starting from the 0th
TEST_F( RingBufferTest, RingBufferAvailableAll ) {
	unsigned size;
	int i;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = rb[ i ].capacity;
		size = rb[ i ].len;
		do_available_test( &rb[ i ], size );
	}
}

// avaiability when size is all elements (head of buffer is in the middle rather than at 0)
TEST_F( RingBufferTest, RingBufferAvailableWrap ) {
	unsigned size;
	int i;
	for( i = 0; i < n; i++ ) {
		rb[ i ].len = rb[ i ].capacity;
		rb[ i ].head = rb[ i ].len / 2;
		size = rb[ i ].len;
		do_available_test( &rb[ i ], size );
	}
}

//
// Tests for ring_buffer_t::reset()
//

TEST_F( RingBufferTest, RingBufferReset ) {

	static const unsigned expected_head = 0;
	static const unsigned expected_len = 0;

	unsigned actual_head;
	unsigned actual_len;

	int i;

	for( i = 0; i < n; i++ ) {
		rb[ i ].len = rb[ i ].capacity;
		rb[ i ].head = rb[ i ].capacity / 2;

		rb[ i ].reset( & rb[ i ] );

		actual_head = rb[ i ].head;
		actual_len = rb[ i ].len;

		EXPECT_EQ( expected_head, actual_head );
		EXPECT_EQ( expected_len, actual_len );
	}
}

