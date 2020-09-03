
#include "storm-pomdp/generator/NondeterministicBeliefTracker.h"
#include "storm/utility/ConstantsComparator.h"

namespace storm {
    namespace generator {

        template<typename ValueType>
        BeliefStateManager<ValueType>::BeliefStateManager(storm::models::sparse::Pomdp<ValueType> const& pomdp)
        : pomdp(pomdp)
        {
            numberActionsPerObservation = std::vector<uint64_t>(pomdp.getNrObservations(), 0);
            for (uint64_t state = 0; state < pomdp.getNumberOfStates(); ++state) {
                numberActionsPerObservation[pomdp.getObservation(state)] = pomdp.getNumberOfChoices(state);
            }
        }

        template<typename ValueType>
        uint64_t BeliefStateManager<ValueType>::getActionsForObservation(uint32_t observation) const {
            return numberActionsPerObservation[observation];
        }

        template<typename ValueType>
        ValueType BeliefStateManager<ValueType>::getRisk(uint64_t state) const {
            return riskPerState.at(state);
        }

        template<typename ValueType>
        storm::models::sparse::Pomdp<ValueType> const& BeliefStateManager<ValueType>::getPomdp() const {
            return pomdp;
        }

        template<typename ValueType>
        void BeliefStateManager<ValueType>::setRiskPerState(std::vector<ValueType> const& risk) {
            riskPerState = risk;
        }

        template<typename ValueType>
        uint64_t BeliefStateManager<ValueType>::getFreshId() {
            beliefIdCounter++;
            std::cout << "provide " << beliefIdCounter;
            return beliefIdCounter;
        }

        template<typename ValueType>
        SparseBeliefState<ValueType>::SparseBeliefState(std::shared_ptr<BeliefStateManager<ValueType>> const& manager, uint64_t state)
        : manager(manager), belief(), id(0), prevId(0)
        {
            id = manager->getFreshId();
            belief[state] = storm::utility::one<ValueType>();
            risk = manager->getRisk(state);
        }

        template<typename ValueType>
        SparseBeliefState<ValueType>::SparseBeliefState(std::shared_ptr<BeliefStateManager<ValueType>> const& manager, std::map<uint64_t, ValueType> const& belief,
                                                        std::size_t hash, ValueType const& risk, uint64_t prevId)
        : manager(manager), belief(belief), prestoredhash(hash), risk(risk), id(0), prevId(prevId)
        {
            id = manager->getFreshId();
        }

        template<typename ValueType>
        ValueType SparseBeliefState<ValueType>::get(uint64_t state) const {
            return belief.at(state);
        }

        template<typename ValueType>
        ValueType SparseBeliefState<ValueType>::getRisk() const {
            return risk;
        }

        template<typename ValueType>
        std::size_t SparseBeliefState<ValueType>::hash() const noexcept {
            return prestoredhash;
        }

        template<typename ValueType>
        bool SparseBeliefState<ValueType>::isValid() const {
            return !belief.empty();
        }

        template<typename ValueType>
        std::string SparseBeliefState<ValueType>::toString() const {
            std::stringstream sstr;
            sstr << "id: " << id << "; ";
            bool first = true;
            for (auto const& beliefentry : belief) {
                if (!first) {
                    sstr << ", ";
                } else {
                    first = false;
                }
                sstr << beliefentry.first << " : " << beliefentry.second;
            }
            sstr << " (from " << prevId << ")";
            return sstr.str();
        }

        template<typename ValueType>
        bool operator==(SparseBeliefState<ValueType> const& lhs, SparseBeliefState<ValueType> const& rhs) {
            if (lhs.hash() != rhs.hash()) {
                return false;
            }
            if (lhs.belief.size() != rhs.belief.size()) {
                return false;
            }
            storm::utility::ConstantsComparator<ValueType> cmp(0.00001, true);
            auto lhsIt = lhs.belief.begin();
            auto rhsIt = rhs.belief.begin();
            while(lhsIt != lhs.belief.end()) {
                if (lhsIt->first != rhsIt->first || !cmp.isEqual(lhsIt->second, rhsIt->second)) {
                    return false;
                }
                ++lhsIt;
                ++rhsIt;
            }
            return true;
            //return std::equal(lhs.belief.begin(), lhs.belief.end(), rhs.belief.begin());
        }

        template<typename ValueType>
        SparseBeliefState<ValueType> SparseBeliefState<ValueType>::update(uint64_t action, uint32_t observation) const {
            std::map<uint64_t, ValueType> newBelief;
            ValueType sum = storm::utility::zero<ValueType>();
            for (auto const& beliefentry : belief) {
                assert(manager->getPomdp().getNumberOfChoices(beliefentry.first) > action);
                auto row = manager->getPomdp().getNondeterministicChoiceIndices()[beliefentry.first] + action;
                for (auto const& transition : manager->getPomdp().getTransitionMatrix().getRow(row)) {
                    if (observation != manager->getPomdp().getObservation(transition.getColumn())) {
                        continue;
                    }

                    if (newBelief.count(transition.getColumn()) == 0) {
                        newBelief[transition.getColumn()] = transition.getValue() * beliefentry.second;
                    } else {
                        newBelief[transition.getColumn()] += transition.getValue() * beliefentry.second;
                    }
                    sum += transition.getValue() * beliefentry.second;
                }
            }
            std::size_t newHash = 0;
            ValueType risk = storm::utility::zero<ValueType>();
            for(auto& entry : newBelief) {
                assert(!storm::utility::isZero(sum));
                entry.second /= sum;
                //boost::hash_combine(newHash, std::hash<ValueType>()(entry.second));
                boost::hash_combine(newHash, entry.first);
                risk += entry.second * manager->getRisk(entry.first);
            }
            return SparseBeliefState<ValueType>(manager, newBelief, newHash, risk, id);
        }

        template<typename ValueType>
        void SparseBeliefState<ValueType>::update(uint32_t newObservation, std::unordered_set<SparseBeliefState<ValueType>>& previousBeliefs) const {
            updateHelper({{}}, {storm::utility::zero<ValueType>()}, belief.begin(), newObservation, previousBeliefs);
        }

        template<typename ValueType>
        void SparseBeliefState<ValueType>::updateHelper(std::vector<std::map<uint64_t, ValueType>> const& partialBeliefs, std::vector<ValueType> const& sums, typename std::map<uint64_t, ValueType>::const_iterator nextStateIt, uint32_t newObservation, std::unordered_set<SparseBeliefState<ValueType>>& previousBeliefs) const {
            if(nextStateIt == belief.end()) {
                for (uint64_t i = 0; i < partialBeliefs.size(); ++i) {
                    auto const& partialBelief = partialBeliefs[i];
                    auto const& sum = sums[i];
                    if (storm::utility::isZero(sum)) {
                        continue;
                    }
                    std::size_t newHash = 0;
                    ValueType risk = storm::utility::zero<ValueType>();
                    std::map<uint64_t, ValueType> finalBelief;
                    for (auto &entry : partialBelief) {
                        assert(!storm::utility::isZero(sum));
                        finalBelief[entry.first] = entry.second / sum;
                        //boost::hash_combine(newHash, std::hash<ValueType>()(entry.second));
                        boost::hash_combine(newHash, entry.first);
                        risk += entry.second / sum * manager->getRisk(entry.first);
                    }
                    previousBeliefs.insert(SparseBeliefState<ValueType>(manager, finalBelief, newHash, risk, id));
                }
            } else {
                uint64_t state = nextStateIt->first;
                auto newNextStateIt = nextStateIt;
                newNextStateIt++;
                std::vector<std::map<uint64_t, ValueType>> newPartialBeliefs;
                std::vector<ValueType> newSums;
                for (uint64_t i = 0; i < partialBeliefs.size(); ++i) {

                    for (auto row = manager->getPomdp().getNondeterministicChoiceIndices()[state];
                         row < manager->getPomdp().getNondeterministicChoiceIndices()[state + 1]; ++row) {
                        std::map<uint64_t, ValueType> newPartialBelief = partialBeliefs[i];
                        ValueType newSum = sums[i];
                        for (auto const &transition : manager->getPomdp().getTransitionMatrix().getRow(row)) {
                            if (newObservation != manager->getPomdp().getObservation(transition.getColumn())) {
                                continue;
                            }

                            if (newPartialBelief.count(transition.getColumn()) == 0) {
                                newPartialBelief[transition.getColumn()] = transition.getValue() * nextStateIt->second;
                            } else {
                                newPartialBelief[transition.getColumn()] += transition.getValue() * nextStateIt->second;
                            }
                            newSum += transition.getValue() * nextStateIt->second;

                        }
                        newPartialBeliefs.push_back(newPartialBelief);
                        newSums.push_back(newSum);
                    }
                }
                updateHelper(newPartialBeliefs, newSums, newNextStateIt, newObservation, previousBeliefs);

            }
        }


        template<typename ValueType, typename BeliefState>
        NondeterministicBeliefTracker<ValueType, BeliefState>::NondeterministicBeliefTracker(storm::models::sparse::Pomdp<ValueType> const& pomdp) :
        pomdp(pomdp), manager(std::make_shared<BeliefStateManager<ValueType>>(pomdp)), beliefs() {
            //
        }

        template<typename ValueType, typename BeliefState>
        bool NondeterministicBeliefTracker<ValueType, BeliefState>::reset(uint32_t observation) {
            bool hit = false;
            for (auto state : pomdp.getInitialStates()) {
                if (observation == pomdp.getObservation(state)) {
                    hit = true;
                    beliefs.emplace(manager, state);
                }
            }
            lastObservation = observation;
            return hit;
        }

        template<typename ValueType, typename BeliefState>
        bool NondeterministicBeliefTracker<ValueType, BeliefState>::track(uint64_t newObservation) {
            STORM_LOG_THROW(!beliefs.empty(), storm::exceptions::InvalidOperationException, "Cannot track without a belief (need to reset).");
            std::unordered_set<BeliefState> newBeliefs;
            //for (uint64_t action = 0; action < manager->getActionsForObservation(lastObservation); ++action) {
            for (auto const& belief : beliefs) {
                belief.update(newObservation, newBeliefs);
            }
            //}
            beliefs = newBeliefs;
            lastObservation = newObservation;
            return !beliefs.empty();
        }

        template<typename ValueType, typename BeliefState>
        ValueType NondeterministicBeliefTracker<ValueType, BeliefState>::getCurrentRisk(bool max) {
            STORM_LOG_THROW(!beliefs.empty(), storm::exceptions::InvalidOperationException, "Risk is only defined for beliefs (run reset() first).");
            ValueType result = beliefs.begin()->getRisk();
            if (max) {
                for (auto const& belief : beliefs) {
                    if (belief.getRisk() > result) {
                        result = belief.getRisk();
                    }
                }
            } else {
                for (auto const& belief : beliefs) {
                    if (belief.getRisk() < result) {
                        result = belief.getRisk();
                    }
                }
            }
            return result;
        }

        template<typename ValueType, typename BeliefState>
        void NondeterministicBeliefTracker<ValueType, BeliefState>::setRisk(std::vector<ValueType> const& risk) {
            manager->setRiskPerState(risk);
        }

        template<typename ValueType, typename BeliefState>
        std::unordered_set<BeliefState> const& NondeterministicBeliefTracker<ValueType, BeliefState>::getCurrentBeliefs() const {
            return beliefs;
        }

        template<typename ValueType, typename BeliefState>
        uint32_t NondeterministicBeliefTracker<ValueType, BeliefState>::getCurrentObservation() const {
            return lastObservation;
        }

        template class SparseBeliefState<double>;
        template bool operator==(SparseBeliefState<double> const&, SparseBeliefState<double> const&);
        template class NondeterministicBeliefTracker<double, SparseBeliefState<double>>;

    }
}
