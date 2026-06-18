//
// Created by core on 18.06.26.
//

#include "factory/parse.hpp"
#include <memory>
#include <exprtk.hpp>

namespace factory
{
    std::function<double(double, double)> parse_theta_phi_function(std::string_view const expression)
    {
        // Struct to hold all ExprTk internal state variables safely on the heap
        struct ExpressionContext
        {
            double theta = 0.0;
            double phi = 0.0;
            exprtk::symbol_table<double> symbol_table;
            exprtk::expression<double> expression;
        };

        // Allocate the context
        auto ctx = std::make_shared<ExpressionContext>();

        // Bind variables to the symbol table
        ctx->symbol_table.add_variable("theta", ctx->theta);
        ctx->symbol_table.add_variable("phi", ctx->phi);
        ctx->symbol_table.add_constants();

        ctx->expression.register_symbol_table(ctx->symbol_table);

        // Compile the string
        exprtk::parser<double> parser;
        if (!parser.compile(std::string(expression), ctx->expression)) { throw std::runtime_error("ExprTk compilation failed: " + parser.error()); }

        // Return the callable lambda.
        // Capturing 'ctx' by value extends the lifetime of the underlying ExprTk objects.
        return [ctx](double const theta, double const phi) -> double
        {
            ctx->theta = theta;
            ctx->phi = phi;
            return ctx->expression.value();
        };
    }
} // namespace factory
