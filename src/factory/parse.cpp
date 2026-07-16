//
// Created by core on 18.06.26.
//

#include "factory/parse.hpp"
#include <complex>
#include <memory>
#include <exprtk.hpp>
#include "simulationerror.hpp"

namespace factory
{
    namespace
    {
        long double parse_long_double(std::string const& expr, std::map<std::string, var_t> const& variables)
        {
            exprtk::symbol_table<long double> symbol_table;
            exprtk::expression<long double> expression;
            for (const auto& [key, val] : variables)
            {
                symbol_table.add_constant(key, std::visit([](auto& v) { return static_cast<long double>(v); }, val));
            }
            symbol_table.add_constants();
            expression.register_symbol_table(symbol_table);
            if (exprtk::parser<long double> parser; !parser.compile(expr, expression))
            {
                throw SimulationError("ExprTk compilation failed: {}", parser.error());
            }
            return expression.value();
        }
    } // namespace

    std::function<complex_t(double polar, double azimuth, double wavelength)> parse_polar_azimuth_function(std::string const& expr)
    {
        // Struct to hold all ExprTk internal state variables safely on the heap
        struct ExpressionContext
        {
            double polar = 0.0;
            double azimuth = 0.0;
            double wavelength = 0.0;
            exprtk::symbol_table<double> symbol_table;
            exprtk::expression<double> expression;
        };

        // Allocate the context
        auto ctx = std::make_shared<ExpressionContext>();

        // Bind variables to the symbol table
        ctx->symbol_table.add_variable("theta", ctx->polar);
        ctx->symbol_table.add_variable("phi", ctx->azimuth);
        ctx->symbol_table.add_variable("lambda", ctx->wavelength);
        ctx->symbol_table.add_constants();

        ctx->expression.register_symbol_table(ctx->symbol_table);

        // Compile the string
        if (exprtk::parser<double> parser; !parser.compile(expr, ctx->expression)) { throw SimulationError("ExprTk compilation failed: {}", parser.error()); }

        // Return the callable lambda.
        // Capturing 'ctx' by value extends the lifetime of the underlying ExprTk objects.
        return [ctx](double const polar, double const azimuth, double const wavelength) -> complex_t
        {
            ctx->polar = polar;
            ctx->azimuth = azimuth;
            ctx->wavelength = wavelength;
            return {ctx->expression.value(), 0.0};
        };
    }

    double parse_double(std::string const& expr, std::map<std::string, var_t> const& variables)
    {
        return static_cast<double>(parse_long_double(expr, variables));
    }

    std::int64_t parse_int(std::string const& expr, std::map<std::string, var_t> const& variables)
    {
        return static_cast<std::int64_t>(std::roundl(parse_long_double(expr, variables)));
    }

} // namespace factory
