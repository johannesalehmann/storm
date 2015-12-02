#include "src/storage/dd/sylvan/InternalSylvanBdd.h"

#include "src/storage/dd/sylvan/InternalSylvanDdManager.h"
#include "src/storage/dd/sylvan/InternalSylvanAdd.h"
#include "src/storage/dd/sylvan/SylvanAddIterator.h"

#include "src/storage/BitVector.h"

#include "src/utility/macros.h"
#include "src/exceptions/NotImplementedException.h"

#include <iostream>

namespace storm {
    namespace dd {
        InternalBdd<DdType::Sylvan>::InternalBdd(InternalDdManager<DdType::Sylvan> const* ddManager, sylvan::Bdd const& sylvanBdd) : ddManager(ddManager), sylvanBdd(sylvanBdd) {
            // Intentionally left empty.
        }
        
        template<typename ValueType>
        InternalBdd<DdType::Sylvan> InternalBdd<DdType::Sylvan>::fromVector(InternalDdManager<DdType::Sylvan> const* ddManager, std::vector<ValueType> const& values, Odd const& odd, std::vector<uint_fast64_t> const& sortedDdVariableIndices, std::function<bool (ValueType const&)> const& filter) {
            STORM_LOG_THROW(false, storm::exceptions::NotImplementedException, "Not yet implemented.");
        }
        
        bool InternalBdd<DdType::Sylvan>::operator==(InternalBdd<DdType::Sylvan> const& other) const {
            return sylvanBdd == other.sylvanBdd;
        }
        
        bool InternalBdd<DdType::Sylvan>::operator!=(InternalBdd<DdType::Sylvan> const& other) const {
            return sylvanBdd != other.sylvanBdd;
        }
        
        InternalBdd<DdType::Sylvan> InternalBdd<DdType::Sylvan>::relationalProduct(InternalBdd<DdType::Sylvan> const& relation, std::vector<InternalBdd<DdType::Sylvan>> const& rowVariables, std::vector<InternalBdd<DdType::Sylvan>> const& columnVariables) const {
            return InternalBdd<DdType::Sylvan>(ddManager, this->sylvanBdd.RelNext(relation.sylvanBdd, sylvan::Bdd(sylvan_false)));
        }

        InternalBdd<DdType::Sylvan> InternalBdd<DdType::Sylvan>::inverseRelationalProduct(InternalBdd<DdType::Sylvan> const& relation, std::vector<InternalBdd<DdType::Sylvan>> const& rowVariables, std::vector<InternalBdd<DdType::Sylvan>> const& columnVariables) const {
            return InternalBdd<DdType::Sylvan>(ddManager, this->sylvanBdd.RelPrev(relation.sylvanBdd, sylvan::Bdd(sylvan_false)));
        }
        
        InternalBdd<DdType::Sylvan> InternalBdd<DdType::Sylvan>::inverseRelationalProductWithExtendedRelation(InternalBdd<DdType::Sylvan> const& relation, std::vector<InternalBdd<DdType::Sylvan>> const& rowVariables, std::vector<InternalBdd<DdType::Sylvan>> const& columnVariables) const {
            // Currently, there is no specialized version to perform this operation, so we fall back to the regular operations.
            
            InternalBdd<DdType::Sylvan> columnCube = ddManager->getBddOne();
            for (auto const& variable : columnVariables) {
                columnCube &= variable;
            }

            return this->swapVariables(rowVariables, columnVariables).andExists(relation, columnCube);
        }
        
        InternalBdd<DdType::Sylvan> InternalBdd<DdType::Sylvan>::ite(InternalBdd<DdType::Sylvan> const& thenDd, InternalBdd<DdType::Sylvan> const& elseDd) const {
            return InternalBdd<DdType::Sylvan>(ddManager, this->sylvanBdd.Ite(thenDd.sylvanBdd, elseDd.sylvanBdd));
        }
        
        InternalBdd<DdType::Sylvan> InternalBdd<DdType::Sylvan>::operator||(InternalBdd<DdType::Sylvan> const& other) const {
            return InternalBdd<DdType::Sylvan>(ddManager, this->sylvanBdd | other.sylvanBdd);
        }
        
        InternalBdd<DdType::Sylvan>& InternalBdd<DdType::Sylvan>::operator|=(InternalBdd<DdType::Sylvan> const& other) {
            this->sylvanBdd |= other.sylvanBdd;
            return *this;
        }
        
        InternalBdd<DdType::Sylvan> InternalBdd<DdType::Sylvan>::operator&&(InternalBdd<DdType::Sylvan> const& other) const {
            return InternalBdd<DdType::Sylvan>(ddManager, this->sylvanBdd & other.sylvanBdd);
        }
        
        InternalBdd<DdType::Sylvan>& InternalBdd<DdType::Sylvan>::operator&=(InternalBdd<DdType::Sylvan> const& other) {
            this->sylvanBdd &= other.sylvanBdd;
            return *this;
        }
        
        InternalBdd<DdType::Sylvan> InternalBdd<DdType::Sylvan>::iff(InternalBdd<DdType::Sylvan> const& other) const {
            return InternalBdd<DdType::Sylvan>(ddManager, !(this->sylvanBdd ^ other.sylvanBdd));
        }
        
        InternalBdd<DdType::Sylvan> InternalBdd<DdType::Sylvan>::exclusiveOr(InternalBdd<DdType::Sylvan> const& other) const {
            return InternalBdd<DdType::Sylvan>(ddManager, this->sylvanBdd ^ other.sylvanBdd);
        }
        
        InternalBdd<DdType::Sylvan> InternalBdd<DdType::Sylvan>::implies(InternalBdd<DdType::Sylvan> const& other) const {
            return InternalBdd<DdType::Sylvan>(ddManager, !this->sylvanBdd | other.sylvanBdd);
        }
        
        InternalBdd<DdType::Sylvan> InternalBdd<DdType::Sylvan>::operator!() const {
            return InternalBdd<DdType::Sylvan>(ddManager, !this->sylvanBdd);
        }
        
        InternalBdd<DdType::Sylvan>& InternalBdd<DdType::Sylvan>::complement() {
            this->sylvanBdd = !this->sylvanBdd;
            return *this;
        }
        
        InternalBdd<DdType::Sylvan> InternalBdd<DdType::Sylvan>::existsAbstract(InternalBdd<DdType::Sylvan> const& cube) const {
            return InternalBdd<DdType::Sylvan>(ddManager, this->sylvanBdd.ExistAbstract(cube.sylvanBdd));
        }
        
        InternalBdd<DdType::Sylvan> InternalBdd<DdType::Sylvan>::universalAbstract(InternalBdd<DdType::Sylvan> const& cube) const {
            return InternalBdd<DdType::Sylvan>(ddManager, this->sylvanBdd.UnivAbstract(cube.sylvanBdd));
        }
        
        InternalBdd<DdType::Sylvan> InternalBdd<DdType::Sylvan>::andExists(InternalBdd<DdType::Sylvan> const& other, InternalBdd<DdType::Sylvan> const& cube) const {
            return InternalBdd<DdType::Sylvan>(ddManager, this->sylvanBdd.AndAbstract(other.sylvanBdd, cube.sylvanBdd));
        }
        
        InternalBdd<DdType::Sylvan> InternalBdd<DdType::Sylvan>::constrain(InternalBdd<DdType::Sylvan> const& constraint) const {
            return InternalBdd<DdType::Sylvan>(ddManager, this->sylvanBdd.Constrain(constraint.sylvanBdd));
        }
        
        InternalBdd<DdType::Sylvan> InternalBdd<DdType::Sylvan>::restrict(InternalBdd<DdType::Sylvan> const& constraint) const {
            return InternalBdd<DdType::Sylvan>(ddManager, this->sylvanBdd.Restrict(constraint.sylvanBdd));
        }
        
        InternalBdd<DdType::Sylvan> InternalBdd<DdType::Sylvan>::swapVariables(std::vector<InternalBdd<DdType::Sylvan>> const& from, std::vector<InternalBdd<DdType::Sylvan>> const& to) const {
            std::vector<uint32_t> fromIndices;
            std::vector<uint32_t> toIndices;
            for (auto it1 = from.begin(), ite1 = from.end(), it2 = to.begin(); it1 != ite1; ++it1, ++it2) {
                fromIndices.push_back(it1->getIndex());
                fromIndices.push_back(it2->getIndex());
                toIndices.push_back(it2->getIndex());
                toIndices.push_back(it1->getIndex());
            }
            return InternalBdd<DdType::Sylvan>(ddManager, this->sylvanBdd.Permute(fromIndices, toIndices));
        }
        
        InternalBdd<DdType::Sylvan> InternalBdd<DdType::Sylvan>::getSupport() const {
            return InternalBdd<DdType::Sylvan>(ddManager, this->sylvanBdd.Support());
        }
        
        uint_fast64_t InternalBdd<DdType::Sylvan>::getNonZeroCount(uint_fast64_t numberOfDdVariables) const {
            if (numberOfDdVariables == 0) {
                return 0;
            }
            return static_cast<uint_fast64_t>(this->sylvanBdd.SatCount(numberOfDdVariables));
        }
        
        uint_fast64_t InternalBdd<DdType::Sylvan>::getLeafCount() const {
            // For BDDs, the leaf count is always one, because the only leaf is the false leaf (and true is represented
            // by a negation edge to false).
            return 1;
        }
        
        uint_fast64_t InternalBdd<DdType::Sylvan>::getNodeCount() const {
            // We have to add one to also count the false-leaf, which is the only leaf appearing in BDDs.
            return static_cast<uint_fast64_t>(this->sylvanBdd.NodeCount()) + 1;
        }
        
        bool InternalBdd<DdType::Sylvan>::isOne() const {
            return this->sylvanBdd.isOne();
        }
        
        bool InternalBdd<DdType::Sylvan>::isZero() const {
            return this->sylvanBdd.isZero();
        }
        
        uint_fast64_t InternalBdd<DdType::Sylvan>::getIndex() const {
            return static_cast<uint_fast64_t>(this->sylvanBdd.TopVar());
        }
        
        void InternalBdd<DdType::Sylvan>::exportToDot(std::string const& filename, std::vector<std::string> const& ddVariableNamesAsStrings) const {
            FILE* filePointer = fopen(filename.c_str() , "w");
            this->sylvanBdd.PrintDot(filePointer);
            fclose(filePointer);
        }
        
        sylvan::Bdd& InternalBdd<DdType::Sylvan>::getSylvanBdd() {
            return sylvanBdd;
        }
        
        sylvan::Bdd const& InternalBdd<DdType::Sylvan>::getSylvanBdd() const {
            return sylvanBdd;
        }
        
        template<typename ValueType>
        InternalAdd<DdType::Sylvan, ValueType> InternalBdd<DdType::Sylvan>::toAdd() const {
            return InternalAdd<DdType::Sylvan, ValueType>(ddManager, this->sylvanBdd.toDoubleMtbdd());
        }
        
        storm::storage::BitVector InternalBdd<DdType::Sylvan>::toVector(storm::dd::Odd const& rowOdd, std::vector<uint_fast64_t> const& ddVariableIndices) const {
            STORM_LOG_THROW(false, storm::exceptions::NotImplementedException, "Not yet implemented.");
        }
        
        Odd InternalBdd<DdType::Sylvan>::createOdd(std::vector<uint_fast64_t> const& ddVariableIndices) const {
            STORM_LOG_THROW(false, storm::exceptions::NotImplementedException, "Not yet implemented.");
        }
        
        template<typename ValueType>
        void InternalBdd<DdType::Sylvan>::filterExplicitVector(Odd const& odd, std::vector<uint_fast64_t> const& ddVariableIndices, std::vector<ValueType> const& sourceValues, std::vector<ValueType>& targetValues) const {
            STORM_LOG_THROW(false, storm::exceptions::NotImplementedException, "Not yet implemented.");
        }
        
        template InternalBdd<DdType::Sylvan> InternalBdd<DdType::Sylvan>::fromVector(InternalDdManager<DdType::Sylvan> const* ddManager, std::vector<double> const& values, Odd const& odd, std::vector<uint_fast64_t> const& sortedDdVariableIndices, std::function<bool (double const&)> const& filter);
        template InternalBdd<DdType::Sylvan> InternalBdd<DdType::Sylvan>::fromVector(InternalDdManager<DdType::Sylvan> const* ddManager, std::vector<uint_fast64_t> const& values, Odd const& odd, std::vector<uint_fast64_t> const& sortedDdVariableIndices, std::function<bool (uint_fast64_t const&)> const& filter);
        
        template InternalAdd<DdType::Sylvan, double> InternalBdd<DdType::Sylvan>::toAdd() const;
        template InternalAdd<DdType::Sylvan, uint_fast64_t> InternalBdd<DdType::Sylvan>::toAdd() const;
                
        template void InternalBdd<DdType::Sylvan>::filterExplicitVector(Odd const& odd, std::vector<uint_fast64_t> const& ddVariableIndices, std::vector<double> const& sourceValues, std::vector<double>& targetValues) const;
        template void InternalBdd<DdType::Sylvan>::filterExplicitVector(Odd const& odd, std::vector<uint_fast64_t> const& ddVariableIndices, std::vector<uint_fast64_t> const& sourceValues, std::vector<uint_fast64_t>& targetValues) const;
    }
}