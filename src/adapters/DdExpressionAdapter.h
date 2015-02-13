#ifndef STORM_ADAPTERS_DDEXPRESSIONADAPTER_H_
#define STORM_ADAPTERS_DDEXPRESSIONADAPTER_H_

#include "src/storage/expressions/Variable.h"
#include "src/storage/expressions/Expressions.h"
#include "storage/expressions/ExpressionVisitor.h"

#include "src/storage/dd/Dd.h"
#include "src/storage/dd/DdManager.h"

namespace storm {
    namespace adapters {
        
        template<storm::dd::DdType Type>
        class SymbolicExpressionAdapter : public storm::expressions::ExpressionVisitor {
        public:
            SymbolicExpressionAdapter(storm::dd::DdManager<Type> const& ddManager, std::map<storm::expressions::Variable, storm::expressions::Variable> const& variableMapping);
            
            storm::dd::Dd<Type> translateExpression(storm::expressions::Expression const& expression);
            
            virtual boost::any visit(storm::expressions::IfThenElseExpression const& expression) override;
            virtual boost::any visit(storm::expressions::BinaryBooleanFunctionExpression const& expression) override;
            virtual boost::any visit(storm::expressions::BinaryNumericalFunctionExpression const& expression) override;
            virtual boost::any visit(storm::expressions::BinaryRelationExpression const& expression) override;
            virtual boost::any visit(storm::expressions::VariableExpression const& expression) override;
            virtual boost::any visit(storm::expressions::UnaryBooleanFunctionExpression const& expression) override;
            virtual boost::any visit(storm::expressions::UnaryNumericalFunctionExpression const& expression) override;
            virtual boost::any visit(storm::expressions::BooleanLiteralExpression const& expression) override;
            virtual boost::any visit(storm::expressions::IntegerLiteralExpression const& expression) override;
            virtual boost::any visit(storm::expressions::DoubleLiteralExpression const& expression) override;

        private:
            // The manager responsible for the DDs built by this adapter.
            storm::dd::DdManager<Type> const& ddManager;
            
            // This member maps the variables used in the expressions to the variables used by the DD manager.
            std::map<storm::expressions::Variable, storm::expressions::Variable> const& variableMapping;
        };
        
    } // namespace adapters
} // namespace storm

#endif /* STORM_ADAPTERS_DDEXPRESSIONADAPTER_H_ */
