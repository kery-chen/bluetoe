#include "test_radio.hpp"
#include "../hexdump.hpp"

#include <boost/test/unit_test.hpp>

namespace test {

    std::ostream& operator<<( std::ostream& out, const schedule_data& data )
    {
        out << "at: " << data.schedule_time << "; scheduled: " << data.transmision_time << "; channel: " << data.channel;
        out << "\nData:\n" << hex_dump( data.transmitted_data.begin(), data.transmitted_data.end() );

        return out;
    }

    const std::vector< schedule_data >& radio_base::scheduling() const
    {
        return transmitted_data_;
    }

    void radio_base::check_scheduling( const std::function< bool ( const schedule_data& ) >& check, const char* message ) const
    {
        unsigned n = 0;

        for ( const schedule_data& data : transmitted_data_ )
        {
            if ( !check( data ) )
            {
                boost::test_tools::predicate_result result( false );
                result.message() << "\nfor " << ( n + 1 ) << "th scheduled action " << data;
                BOOST_CHECK( result );
                return;
            }

            ++n;
        }
    }

    void radio_base::check_scheduling( const std::function< bool ( const schedule_data& first, const schedule_data& next ) >& check, const char* message ) const
    {
        check_scheduling(
            [&]( const schedule_data& ) { return true; },
            check,
            message
        );
    }

    void radio_base::check_scheduling( const std::function< bool ( const schedule_data& ) >& filter, const std::function< bool ( const schedule_data& first, const schedule_data& next ) >& check, const char* message ) const
    {
        pair_wise_check(
            filter,
            check,
            [&]( data_list::const_iterator first, data_list::const_iterator next )
            {
                const auto n  = std::distance( transmitted_data_.begin(), first ) + 1;
                const auto nn = std::distance( first, next ) + n;

                boost::test_tools::predicate_result result( false );
                result.message() << "\nfor " << n << "th and " << nn << "th scheduled action";
                result.message() << "\nTesting: \"" << message << "\" failed.";
                result.message() << "\n" << n << "th scheduled action, " << *first;
                result.message() << "\n" << nn << "th scheduled action, " << *next;
                BOOST_CHECK( result );
            }
        );
    }

    std::vector< schedule_data >::const_iterator radio_base::next( std::vector< schedule_data >::const_iterator first, const std::function< bool ( const schedule_data& ) >& filter ) const
    {
        while ( first != transmitted_data_.end() && !filter( *first ) )
            ++first;

        return first;
    }

    void radio_base::pair_wise_check(
        const std::function< bool ( const schedule_data& ) >&                                               filter,
        const std::function< bool ( const schedule_data& first, const schedule_data& next ) >&              check,
        const std::function< void ( data_list::const_iterator first, data_list::const_iterator next ) >&    fail ) const
    {
        for ( auto data = next( transmitted_data_.begin(), filter ); data != transmitted_data_.end(); )
        {
            const auto next_data = next( data +1, filter );

            if ( next_data != transmitted_data_.end() )
            {
                if ( !check( *data, *next_data ) )
                {
                    fail( data, next_data );
                    return;
                }
            }

            data = next_data;
        }
    }

    void radio_base::all_data( std::function< void ( const schedule_data& ) > f ) const
    {
        for ( const schedule_data& data : transmitted_data_ )
            f( data );
    }

    void radio_base::all_data( const std::function< bool ( const schedule_data& ) >& filter, const std::function< void ( const schedule_data& first, const schedule_data& next ) >& iter ) const
    {
        pair_wise_check(
            filter,
            [&]( const schedule_data& first, const schedule_data& next )
            {
                iter( first, next );
                return true;
            },
            []( data_list::const_iterator first, data_list::const_iterator next ){}
        );
    }
}
