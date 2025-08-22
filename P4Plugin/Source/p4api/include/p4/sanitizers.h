/*
 * Copyright 1995, 2019 Perforce Software.  All rights reserved.
 *
 * This file is part of Perforce - the FAST SCM System.
 */

# ifndef P4SANITIZERS_H
# define P4SANITIZERS_H

// There's no feature test for UBSAN, so assume that if we've got one,
// we've got the other.

// Clang.
# if defined(__has_feature)
# if __has_feature(address_sanitizer)
# define CAN_SANITIZE
# endif
# endif

// GCC
# if defined(__GNUG__) && defined(__SANITIZE_ADDRESS__)
# define CAN_SANITIZE
# endif

# ifdef CAN_SANITIZE
# undef CAN_SANITIZE
// Note that it's possible to specify specific parts of the sanitizers to disable,
// so if a function is only offending a particular check, we can disable them
// more selectively if necessary.
# define NO_SANITIZE_ADDRESS_UNDEFINED __attribute__((no_sanitize("address","undefined")))
# define NO_SANITIZE_ADDRESS __attribute__((no_sanitize("address")))
# define NO_SANITIZE_UNDEFINED __attribute__((no_sanitize("undefined")))
# else
// NO-OP definitions
# define NO_SANITIZE_ADDRESS_UNDEFINED
# define NO_SANITIZE_ADDRESS
# define NO_SANITIZE_UNDEFINED
# endif // CAN_SANITIZE

# endif // P4SANITIZERS_H
