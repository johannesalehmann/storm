#include "storm/storage/memorystructure/MemoryStructureBuilder.h"

#include "storm/models/sparse/Model.h"
#include "storm/storage/BitVector.h"
#include "storm/utility/macros.h"

#include "storm/exceptions/InvalidOperationException.h"

namespace storm {
    namespace storage {
        
        template <typename ValueType, typename RewardModelType>
        MemoryStructureBuilder<ValueType, RewardModelType>::MemoryStructureBuilder(uint_fast64_t numberOfMemoryStates, storm::models::sparse::Model<ValueType, RewardModelType> const& model) : model(model), transitions(numberOfMemoryStates, std::vector<boost::optional<storm::storage::BitVector>>(numberOfMemoryStates)), stateLabeling(numberOfMemoryStates), initialMemoryStates(model.getInitialStates().getNumberOfSetBits(), 0) {
            // Intentionally left empty
        }
        
        template <typename ValueType, typename RewardModelType>
        void MemoryStructureBuilder<ValueType, RewardModelType>::setInitialMemoryState(uint_fast64_t initialModelState, uint_fast64_t initialMemoryState) {
            STORM_LOG_THROW(model.getInitialStates().get(initialModelState), storm::exceptions::InvalidOperationException, "Invalid index of initial model state: " << initialMemoryState << ". This is not an initial state of the model.");
            STORM_LOG_THROW(initialMemoryState < transitions.size(), storm::exceptions::InvalidOperationException, "Invalid index of initial memory state: " << initialMemoryState << ". There are only " << transitions.size() << " states in this memory structure.");
            
            auto initMemStateIt = initialMemoryStates.begin();
            for (auto const& initState : model.getInitialStates()) {
                if (initState == initialModelState) {
                    *initMemStateIt = initialMemoryState;
                    break;
                }
                ++initMemStateIt;
            }
            assert(initMemStateIt != initialMemoryStates.end());
        }
        
        template <typename ValueType, typename RewardModelType>
        void MemoryStructureBuilder<ValueType, RewardModelType>::setTransition(uint_fast64_t const& startState, uint_fast64_t const& goalState, storm::storage::BitVector const& modelStates, boost::optional<storm::storage::BitVector> const& modelChoices) {
            
            auto const& modelTransitions = model.getTransitionMatrix();
            
            STORM_LOG_THROW(startState < transitions.size(), storm::exceptions::InvalidOperationException, "Invalid index of start state: " << startState << ". There are only " << transitions.size() << " states in this memory structure.");
            STORM_LOG_THROW(goalState < transitions.size(), storm::exceptions::InvalidOperationException, "Invalid index of goal state: " << startState << ". There are only " << transitions.size() << " states in this memory structure.");
            STORM_LOG_THROW(modelStates.size() == modelTransitions.getRowGroupCount(), storm::exceptions::InvalidOperationException, "The modelStates have invalid size.");
            STORM_LOG_THROW(!modelChoices || modelChoices->size() == modelTransitions.getRowGroupCount(), storm::exceptions::InvalidOperationException, "The modelChoices have invalid size.");
            
            // translate the two bitvectors to a single BitVector that indicates the corresponding model transitions.
            
            storm::storage::BitVector transitionVector(modelTransitions.getEntryCount(), false);
            if (modelChoices) {
                for (auto const& choice : modelChoices.get()) {
                    for (auto entryIt = modelTransitions.getRow(choice).begin(); entryIt < modelTransitions.getRow(choice).end(); ++entryIt) {
                        if (modelStates.get(entryIt->getColumn())) {
                            transitionVector.set(entryIt - modelTransitions.begin());
                        }
                    }
                }
            } else {
                for (uint_fast64_t choice = 0; choice < modelTransitions.getRowCount(); ++choice) {
                    for (auto entryIt = modelTransitions.getRow(choice).begin(); entryIt < modelTransitions.getRow(choice).end(); ++entryIt) {
                        if (modelStates.get(entryIt->getColumn())) {
                            transitionVector.set(entryIt - modelTransitions.begin());
                        }
                    }
                }
            }
            
            // Do not insert the transition if it is never taken.
            if (transitionVector.empty()) {
                transitions[startState][goalState] = boost::none;
            } else {
                transitions[startState][goalState] = std::move(transitionVector);
            }
        }

        template <typename ValueType, typename RewardModelType>
        void MemoryStructureBuilder<ValueType, RewardModelType>::setLabel(uint_fast64_t const& state, std::string const& label) {
            STORM_LOG_THROW(state < transitions.size(), storm::exceptions::InvalidOperationException, "Can not add label to state with index " << state << ". There are only " << transitions.size() << " states in this memory structure.");
            if (!stateLabeling.containsLabel(label)) {
                stateLabeling.addLabel(label);
            }
            stateLabeling.addLabelToState(label, state);
        }
        
        template <typename ValueType, typename RewardModelType>
        MemoryStructure MemoryStructureBuilder<ValueType, RewardModelType>::build() {
            return MemoryStructure(std::move(transitions), std::move(stateLabeling), std::move(initialMemoryStates));
        }
        
        template class MemoryStructureBuilder<double>;
        template class MemoryStructureBuilder<storm::RationalNumber>;
        template class MemoryStructureBuilder<storm::RationalFunction>;
        
    }
}