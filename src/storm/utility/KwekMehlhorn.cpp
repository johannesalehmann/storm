#include "storm/utility/KwekMehlhorn.h"

#include "storm/adapters/RationalNumberAdapter.h"

#include "storm/utility/constants.h"
#include "storm/utility/macros.h"

#include "storm/exceptions/PrecisionExceededException.h"

namespace storm {
    namespace utility{
        namespace kwek_mehlhorn {
            
            template<typename IntegerType>
            std::pair<IntegerType, IntegerType> findRational(IntegerType const& alpha, IntegerType const& beta, IntegerType const& gamma, IntegerType const& delta) {
                std::pair<IntegerType, IntegerType> alphaDivBetaPair = storm::utility::divide(alpha, beta);
                std::pair<IntegerType, IntegerType> gammaDivDeltaPair = storm::utility::divide(gamma, delta);
                
                if (alphaDivBetaPair.first == gammaDivDeltaPair.first && !storm::utility::isZero(alphaDivBetaPair.second)) {
                    std::pair<IntegerType, IntegerType> subresult = findRational(delta, gammaDivDeltaPair.second, beta, alphaDivBetaPair.second);
                    auto result = std::make_pair(alphaDivBetaPair.first * subresult.first + subresult.second, subresult.first);
                    
                    return result;
                } else {
                    auto result = std::make_pair(storm::utility::isZero(alphaDivBetaPair.second) ? alphaDivBetaPair.first : alphaDivBetaPair.first + storm::utility::one<IntegerType>(), storm::utility::one<IntegerType>());
                    return result;
                }
            }
            
            template<typename RationalType, typename ImpreciseType>
            std::pair<typename NumberTraits<RationalType>::IntegerType, typename NumberTraits<RationalType>::IntegerType> truncateToRational(ImpreciseType const& value, uint64_t precision) {
                typedef typename NumberTraits<RationalType>::IntegerType IntegerType;
                
                IntegerType powerOfTen = storm::utility::pow(storm::utility::convertNumber<IntegerType>(10ull), precision);
                IntegerType truncated = storm::utility::trunc<RationalType>(value * powerOfTen);
                return std::make_pair(truncated, powerOfTen);
            }
            
            template<typename RationalType>
            std::pair<typename NumberTraits<RationalType>::IntegerType, typename NumberTraits<RationalType>::IntegerType> truncateToRational(double const& value, uint64_t precision) {
                if (precision >= 18) {
                    throw storm::exceptions::PrecisionExceededException() << "Exceeded precision of double, consider switching to rational numbers.";
                }
                
                double powerOfTen = std::pow(10, precision);
                double truncated = storm::utility::trunc<double>(value * powerOfTen);
                return std::make_pair(truncated, powerOfTen);
            }
            
            template<typename RationalType, typename ImpreciseType>
            RationalType findRational(uint64_t precision, ImpreciseType const& value) {
                typedef typename NumberTraits<RationalType>::IntegerType IntegerType;
                
                std::pair<IntegerType, IntegerType> truncatedFraction = truncateToRational<RationalType>(value, precision);
                std::pair<IntegerType, IntegerType> result = findRational<IntegerType>(truncatedFraction.first, truncatedFraction.second, truncatedFraction.first + storm::utility::one<IntegerType>(), truncatedFraction.second);
                
                // Convert one of the arguments to a rational type to not get integer division.
                return storm::utility::convertNumber<RationalType>(result.first) / result.second;
            }
            
            template<typename RationalType, typename ImpreciseType>
            RationalType sharpen(uint64_t precision, ImpreciseType const& value) {
                ImpreciseType integer = storm::utility::floor(value);
                ImpreciseType fraction = value - integer;
                auto rational = findRational<RationalType>(precision, fraction);
                return storm::utility::convertNumber<RationalType>(integer) + rational;
            }
            
            template<typename RationalType, typename ImpreciseType>
            void sharpen(uint64_t precision, std::vector<ImpreciseType> const& input, std::vector<RationalType>& output) {
                for (uint64_t index = 0; index < input.size(); ++index) {
                    output[index] = sharpen<RationalType, ImpreciseType>(precision, input[index]);
                }
            }
         
            template void sharpen(uint64_t precision, std::vector<double> const& input, std::vector<storm::RationalNumber>& output);
            template void sharpen(uint64_t precision, std::vector<storm::RationalNumber> const& input, std::vector<storm::RationalNumber>& output);
            
        }
    }
}