#ifndef PACKED_H_
#define PACKED_H_

#undef PACKED_DEFINED_

#if ! defined( PACKED_DEFINED_ ) && defined(__IAR_SYSTEMS_ICC__)
#define PACKED_IAR __packed
#define PACKED_GNU
#define PACKED_DEFINED_
#endif // ! defined( PACKED_DEFINED_ ) && defined(__IAR_SYSTEMS_ICC__)

#if ! defined( PACKED_DEFINED_ ) && defined(__GNU__)
#define PACKED_GNU __attribute__((packed))
#define PACKED_IAR
#define PACKED_DEFINED_
#endif // ! defined( PACKED_DEFINED_ ) && defined(__GNU__)

#if ! defined( PACKED_DEFINED_ )
#define PACKED_GNU __attribute__((packed))
#define PACKED_IAR
#define PACKED_DEFINED_
#endif // ! defined( PACKED_DEFINED_ )

#endif /* PACKED_H_ */
