#include <NumCpp.hpp>
#include <execution>
#include "print.hpp"
#include <dplot.hpp>
#include <ranges>

#include "bitmap.hpp"
#include "components/hertziandipole.hpp"
#include "manifest.hpp"
#include "plot.hpp"
#include "setup.hpp"
#include "types.hpp"

// #include "ula.hpp"

/*
void compute_rect(array_t const &x_values, array_t const &y_values, std::vector<IRadiator*> const & rad, double freq)
{
    // nc::NdArray<double> simData = nc::random::rand<double>({10, 100});

    // 2. NumCpp NdArrays can be converted to std::vector easily
    // Matplot++ heatmap accepts a std::vector<std::vector<double>>
    // std::vector<std::vector<double>> plotData;
    //
    // for (std::size_t x_idx = 0; x_idx < x_values.size(); x_idx++)
    // {
    //     plotData.emplace_back(y_values.size());
    //     for (std::size_t y_idx = 0; y_idx < y_values.size(); y_idx++)
    //     {
    //         float v = 0;
    //         for (IRadiator const* radiator : rad)
    //         {
    //             v += radiator->eval(x_values[x_idx], y_values[y_idx]);
    //         }
    //         plotData[x_idx][y_idx] = v;
    //     }
    // }

    std::print("computing data\n");
    std::vector<std::vector<double>> plotData(y_values.size(), std::vector<double>(x_values.size()));

    // 2. Create a range of indices (0 to x_size)
    auto x_indices = std::views::iota(0u, x_values.size());

    Bitmap bitmap(x_values.size(), y_values.size());

    // 3. Parallel for_each on the outer loop
    std::for_each(std::execution::par, x_indices.begin(), x_indices.end(), [&](std::size_t x_idx) {
        for (std::size_t y_idx = 0; y_idx < y_values.size(); y_idx++) {
            complex_t v{0,0};
            for (IRadiator const* radiator : rad) {
                v += radiator->get_gain(complex_t(x_values[x_idx], y_values[y_idx]), freq);
            }
            plotData[y_idx][x_idx] = std::abs(v);
            auto lum = std::log10(std::abs(v));
            bitmap[x_idx, y_idx] = {lum, lum, lum};
        }
    });

    std::println("saving bitmap");
    bitmap.clamp(1e-16, 1e16);
    bitmap.normalize();
    bitmap.write("result.bmp");

    return;

    std::println("plotting results");

    // for (size_t i = 0; i < simData.numRows(); ++i)
    // {
    //     plotData.emplace_back(simData.row(i).toStlVector());
    // }

    // 3. Plot
    auto p = matplot::pcolor(plotData);
    matplot::grid(matplot::off);
    matplot::box(matplot::off);
    matplot::colormap(matplot::palette::summer());
    matplot::colorbar(); // Adds a scale so you know what the colors mean

    // 3. Customize the look
    matplot::title("Simulation Results");
    matplot::xlabel("X-Axis (Tiles)");
    matplot::ylabel("Y-Axis (Tiles)");

    // Change colormap (e.g., jet, parula, hot, cool)
    matplot::colormap(matplot::palette::cool());

    matplot::show(); // This opens the window
}
*/

int main(int argc, char* argv[])
{
    std::cout << BANNER;
    std::cout << APPLICATION_NAME << " v." << APPLICATION_VERSION << "\n\n";

    //if (argc == 1)
    //{
    //    std::print("missing n parameter\n");
    //    return 1;
    //}

    auto setup = Setup::from_file("../setup.json");
    setup.export_to_three();
    setup.run_tasks();

    return 0;


    std::size_t n = std::stoi(argv[1]);
    std::print("using {} values per dimension\n", n);


    double freq = 1e9;
    double lambda = SPEED_OF_LIGHT / freq;
    std::print("lambda: {:.02f} m\n", lambda);

    /*

    // auto r1 = IsotropicRadiator(complex_t{0, 0}, 0, freq);
    // auto r2 = TestRadiator(complex_t{0, 1}, std::numbers::pi/4);
    auto r3 = ULA<IsotropicRadiator>{complex_t{0, 0}, 0*std::numbers::pi/4, 8, lambda/2};
    std::vector<IRadiator*> const radiators = {&r3};

    double s = 100;

    compute_rect(
        nc::linspace(-s, s, n),
        nc::linspace(-s, s, n),
        radiators,
        freq);
    */

    return 0;


}
