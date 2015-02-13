#include "src/storage/dd/CuddDdForwardIterator.h"
#include "src/storage/dd/CuddDdManager.h"
#include "src/storage/dd/DdMetaVariable.h"
#include "src/utility/macros.h"

namespace storm {
    namespace dd {
        DdForwardIterator<DdType::CUDD>::DdForwardIterator() : ddManager(), generator(), cube(), value(), isAtEnd(), metaVariables(), enumerateDontCareMetaVariables(), cubeCounter(), relevantDontCareDdVariables(), currentValuation() {
            // Intentionally left empty.
        }
        
        DdForwardIterator<DdType::CUDD>::DdForwardIterator(std::shared_ptr<DdManager<DdType::CUDD> const> ddManager, DdGen* generator, int* cube, double value, bool isAtEnd, std::set<storm::expressions::Variable> const* metaVariables, bool enumerateDontCareMetaVariables) : ddManager(ddManager), generator(generator), cube(cube), value(value), isAtEnd(isAtEnd), metaVariables(metaVariables), enumerateDontCareMetaVariables(enumerateDontCareMetaVariables), cubeCounter(), relevantDontCareDdVariables(), currentValuation(ddManager->getExpressionManager().getSharedPointer()) {
            // If the given generator is not yet at its end, we need to create the current valuation from the cube from
            // scratch.
            if (!this->isAtEnd) {
                // And then get ready for treating the first cube.
                this->treatNewCube();
            }
        }
        
        DdForwardIterator<DdType::CUDD>::DdForwardIterator(DdForwardIterator<DdType::CUDD>&& other) : ddManager(other.ddManager), generator(other.generator), cube(other.cube), value(other.value), isAtEnd(other.isAtEnd), metaVariables(other.metaVariables), cubeCounter(other.cubeCounter), relevantDontCareDdVariables(other.relevantDontCareDdVariables), currentValuation(other.currentValuation) {
            // Null-out the pointers of which we took possession.
            other.cube = nullptr;
            other.generator = nullptr;
        }
        
        DdForwardIterator<DdType::CUDD>& DdForwardIterator<DdType::CUDD>::operator=(DdForwardIterator<DdType::CUDD>&& other) {
            if (this != &other) {
                this->ddManager = other.ddManager;
                this->generator = other.generator;
                this->cube = other.cube;
                this->value = other.value;
                this->isAtEnd = other.isAtEnd;
                this->metaVariables = other.metaVariables;
                this->cubeCounter = other.cubeCounter;
                this->relevantDontCareDdVariables = other.relevantDontCareDdVariables;
                this->currentValuation = other.currentValuation;
                
                // Null-out the pointers of which we took possession.
                other.cube = nullptr;
                other.generator = nullptr;
            }
            return *this;
        }
        
        DdForwardIterator<DdType::CUDD>::~DdForwardIterator() {
            // We free the pointers sind Cudd allocates them using malloc rather than new/delete.
            if (this->cube != nullptr) {
                free(this->cube);
            }
            if (this->generator != nullptr) {
                free(this->generator);
            }
        }
        
        DdForwardIterator<DdType::CUDD>& DdForwardIterator<DdType::CUDD>::operator++() {
            STORM_LOG_ASSERT(!this->isAtEnd, "Illegally incrementing iterator that is already at its end.");
            
            // If there were no (relevant) don't cares or we have enumerated all combination, we need to eliminate the
            // found solutions and get the next "first" cube.
            if (this->relevantDontCareDdVariables.empty() || this->cubeCounter >= std::pow(2, this->relevantDontCareDdVariables.size()) - 1) {
                // Get the next cube and check for emptiness.
                ABDD::NextCube(generator, &cube, &value);
                this->isAtEnd = (Cudd_IsGenEmpty(generator) != 0);

                // In case we are not done yet, we get ready to treat the next cube.
                if (!this->isAtEnd) {
                    this->treatNewCube();
                }
            } else {
                // Treat the next solution of the cube.
                this->treatNextInCube();
            }
            
            return *this;
        }
        
        void DdForwardIterator<DdType::CUDD>::treatNextInCube() {
            // Start by increasing the counter and check which bits we need to set/unset in the new valuation.
            ++this->cubeCounter;
            
            for (uint_fast64_t index = 0; index < this->relevantDontCareDdVariables.size(); ++index) {
                auto const& ddMetaVariable = this->ddManager->getMetaVariable(std::get<1>(this->relevantDontCareDdVariables[index]));
                if (ddMetaVariable.getType() == DdMetaVariable<DdType::CUDD>::MetaVariableType::Bool) {
                    if ((this->cubeCounter & (1ull << index)) != 0) {
                        currentValuation.setBooleanValue(std::get<1>(this->relevantDontCareDdVariables[index]), true);
                    } else {
                        currentValuation.setBooleanValue(std::get<1>(this->relevantDontCareDdVariables[index]), false);
                    }
                } else {
                    storm::expressions::Variable const& metaVariable = std::get<1>(this->relevantDontCareDdVariables[index]);
                    if ((this->cubeCounter & (1ull << index)) != 0) {
                        currentValuation.setBitVectorValue(metaVariable, ((currentValuation.getBitVectorValue(metaVariable) - ddMetaVariable.getLow()) | (1ull << std::get<2>(this->relevantDontCareDdVariables[index]))) + ddMetaVariable.getLow());
                    } else {
                        currentValuation.setBitVectorValue(metaVariable, ((currentValuation.getBitVectorValue(metaVariable) - ddMetaVariable.getLow()) & ~(1ull << std::get<2>(this->relevantDontCareDdVariables[index]))) + ddMetaVariable.getLow());
                    }
                }
            }
        }
        
        void DdForwardIterator<DdType::CUDD>::treatNewCube() {
            this->relevantDontCareDdVariables.clear();
            
            // Now loop through the bits of all meta variables and check whether they need to be set, not set or are
            // don't cares. In the latter case, we add them to a special list, so we can iterate over their concrete
            // valuations later.
            for (auto const& metaVariable : *this->metaVariables) {
                bool metaVariableAppearsInCube = false;
                std::vector<std::tuple<ADD, storm::expressions::Variable, uint_fast64_t>> localRelenvantDontCareDdVariables;
                auto const& ddMetaVariable = this->ddManager->getMetaVariable(metaVariable);
                if (ddMetaVariable.getType() == DdMetaVariable<DdType::CUDD>::MetaVariableType::Bool) {
                    if (this->cube[ddMetaVariable.getDdVariables()[0].getCuddAdd().NodeReadIndex()] == 0) {
                        metaVariableAppearsInCube = true;
                        currentValuation.setBooleanValue(metaVariable, false);
                    } else if (this->cube[ddMetaVariable.getDdVariables()[0].getCuddAdd().NodeReadIndex()] == 1) {
                        metaVariableAppearsInCube = true;
                        currentValuation.setBooleanValue(metaVariable, true);
                    } else {
                        localRelenvantDontCareDdVariables.push_back(std::make_tuple(ddMetaVariable.getDdVariables()[0].getCuddAdd(), metaVariable, 0));
                    }
                } else {
                    int_fast64_t intValue = 0;
                    for (uint_fast64_t bitIndex = 0; bitIndex < ddMetaVariable.getNumberOfDdVariables(); ++bitIndex) {
                        if (cube[ddMetaVariable.getDdVariables()[bitIndex].getCuddAdd().NodeReadIndex()] == 0) {
                            // Leave bit unset.
                            metaVariableAppearsInCube = true;
                        } else if (cube[ddMetaVariable.getDdVariables()[bitIndex].getCuddAdd().NodeReadIndex()] == 1) {
                            intValue |= 1ull << (ddMetaVariable.getNumberOfDdVariables() - bitIndex - 1);
                            metaVariableAppearsInCube = true;
                        } else {
                            // Temporarily leave bit unset so we can iterate trough the other option later.
                            // Add the bit to the relevant don't care bits.
                            localRelenvantDontCareDdVariables.push_back(std::make_tuple(ddMetaVariable.getDdVariables()[bitIndex].getCuddAdd(), metaVariable, ddMetaVariable.getNumberOfDdVariables() - bitIndex - 1));
                        }
                    }
                    if (this->enumerateDontCareMetaVariables || metaVariableAppearsInCube) {
                        currentValuation.setBitVectorValue(metaVariable, intValue + ddMetaVariable.getLow());
                    }
                }
                
                // If all meta variables are to be enumerated or the meta variable appeared in the cube, we register the
                // missing bits to later enumerate all possible valuations.
                if (this->enumerateDontCareMetaVariables || metaVariableAppearsInCube) {
                    relevantDontCareDdVariables.insert(relevantDontCareDdVariables.end(), localRelenvantDontCareDdVariables.begin(), localRelenvantDontCareDdVariables.end());
                }
            }
            
            // Finally, reset the cube counter.
            this->cubeCounter = 0;
        }
        
        bool DdForwardIterator<DdType::CUDD>::operator==(DdForwardIterator<DdType::CUDD> const& other) const {
            if (this->isAtEnd && other.isAtEnd) {
                return true;
            } else {
                return this->ddManager.get() == other.ddManager.get() && this->generator == other.generator
                && this->cube == other.cube && this->value == other.value && this->isAtEnd == other.isAtEnd
                && this->metaVariables == other.metaVariables && this->cubeCounter == other.cubeCounter
                && this->relevantDontCareDdVariables == other.relevantDontCareDdVariables
                && this->currentValuation == other.currentValuation;
            }
        }
        
        bool DdForwardIterator<DdType::CUDD>::operator!=(DdForwardIterator<DdType::CUDD> const& other) const {
            return !(*this == other);
        }
        
        std::pair<storm::expressions::SimpleValuation, double> DdForwardIterator<DdType::CUDD>::operator*() const {
            return std::make_pair(currentValuation, value);
        }
    }
}