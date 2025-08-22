/*
 * Copyright 1995, 1996 Perforce Software.  All rights reserved.
 *
 * This file is part of Perforce - the FAST SCM System.
 */

/*
 * DateTime - get and set the date as a string
 */

// size for Fmt, FmtDay

# define DateTimeBufSize 20 

// size for FmtTz, which can say things like '0700 - Pacific Standard Time'

# define DateTimeZoneBufSize 80 

class DateTime {

    public:
		DateTime() {}
		DateTime( const int date ) { Set( date ); }
		DateTime( const char *date, Error *e ) { Set( date, e ); }

 	void	Set( const char *date, Error *e );
 	void	Set( const P4INT64 date ) { wholeDay = 0; tval = (time_t)date; }
	void	SetNow() { Set( (int)Now() ); }

	P4INT64	Compare( const DateTime &t2 ) const { 
	        return (tval - t2.tval); };

	void	Fmt( char *buf ) const;
	void	FmtDay( char *buf ) const;
	void	FmtDayUTC( char *buf ) const;
	void	FmtTz( char *buf ) const;
	void	FmtUTC( char *buf ) const;
	void 	FmtElapsed( char *buf, const DateTime &t2 );
	void	FmtUnifiedDiff( char *buf ) const;
	void	FmtISO8601( char *buf ) const;
	void	FmtISO8601Min( char *buf ) const;
	
	void	SetRFC5322( const char *date, Error *e );
	void	FmtRFC5322( char *buf ) const;

	void	SetGit( const StrPtr &gitDate, Error *e );
	void	FmtGit( StrBuf &buf ) const;

	P4INT64	Value() const { return tval; }
	P4INT64	Tomorrow() const { return tval + 24*60*60; }
	int	IsWholeDay() const { return wholeDay; }

	static P4INT64 Never() { return 0; }

	// for stat() and utime() conversion

	static P4INT64 Localize( time_t centralTime );
	static P4INT64 Centralize( time_t localTime );	
	P4INT64    TzOffset( int *isdst = 0 ) const;

    protected:
	P4INT64	Now();

    private:
	P4INT64	tval;
	int	wholeDay;

	P4INT64	ParseOffset( const char *s, const char *odate, Error *e );
};

class DateTimeNow : public DateTime {

    public:
		DateTimeNow() { Set( Now() ); }

} ;

// Pass a buffer of at least this size to DateTimeHighPrecision::Fmt():

# define DTHighPrecisionBufSize 40 

/*
 * Uses gettimeofday/clock_gettime/etc. to find more precise system time
 */
class DateTimeHighPrecision
{
    public:

	// Orthodox Canonical Form (OCF) methods (we don't need a dtor)
	        DateTimeHighPrecision(P4INT64 secs = 0, int nsecs = 0)
		    : seconds( secs ), nanos( nsecs ) { }

	        DateTimeHighPrecision(const DateTimeHighPrecision &rhs)
		    : seconds( rhs.seconds ), nanos( rhs.nanos ) { }

	DateTimeHighPrecision &
		operator=( const DateTimeHighPrecision &rhs );

	DateTimeHighPrecision &
		operator+=( const DateTimeHighPrecision &rhs );

	DateTimeHighPrecision &
		operator-=( const DateTimeHighPrecision &rhs );

	bool
	operator==(
		const DateTimeHighPrecision &rhs) const;

	bool
	operator!=(
		const DateTimeHighPrecision &rhs) const;

	bool
	operator<(
		const DateTimeHighPrecision &rhs) const;

	bool
	operator<=(
		const DateTimeHighPrecision &rhs) const;

	bool
	operator>(
		const DateTimeHighPrecision &rhs) const;

	bool
	operator>=(
		const DateTimeHighPrecision &rhs) const;

	void	Now();
	void	Fmt( char *buf ) const;
	void	FmtISO8601( char *buf ) const;

	P4INT64	Seconds() const;
	P4INT64	Nanos() const;

	bool	IsZero() const { return seconds == 0 && nanos == 0; }

	// return (t2 - *this) in nanoseconds
	P4INT64 ElapsedNanos( const DateTimeHighPrecision &t2 ) const;

	void	FmtElapsed( StrBuf &buf, const DateTimeHighPrecision t2 ) const;
	// return < 0, = 0, or > 0 if *this < rhs, *this == rhs, or *this > rhs, respectively
	int 	Compare( const DateTimeHighPrecision &rhs ) const;

	P4INT64	ToNanos() const;
	P4INT64	ToMs() const;

    private:

	P4INT64	seconds; // Since 1/1/1970, natch
	int	nanos;
} ;

