#ifndef STORM_STORAGE_GSPN_IMMEDIATETRANSITION_H_
#define STORM_STORAGE_GSPN_IMMEDIATETRANSITION_H_

#include "src/storage/gspn/Transition.h"
#include "src/utility/constants.h"

namespace storm {
    namespace gspn {
        template <typename WeightType>
        class ImmediateTransition : public storm::gspn::Transition {
        public:
            /*!
             * Sets the weight of this transition to the given value.
             *
             * @param weight The new weight for this transition.
             */
            void setWeight(WeightType const& weight) {
                this->weight = weight;
            }

            /*!
             * Retrieves the weight of this transition.
             *
             * @return The weight of this transition.
             */
            WeightType getWeight() const {
                return this->weight;
            }

            /**
             * True iff no weight is attached.
             */
            bool noWeightAttached() const {
                return storm::utility::isZero(weight);
            }
        private:
            // the weight of the transition. Must be positive, if zero, then no weight is attached.
           WeightType weight;
        };
    }
}

#endif //STORM_STORAGE_GSPN_IMMEDIATETRANSITION_H_
