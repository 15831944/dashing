/*
Copyright (c) 2015 Jeff Epler

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgement in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

// demo and benchmark program for dashing

#include "dashing.h"
#include "parse_numbers.h"
#include <iostream>
#include <boost/random.hpp>
#include <boost/random/random_device.hpp>

using namespace dashing;

std::vector<Segment> SegmentsFromFile(std::istream &fi, double jitter) {
    static boost::random_device urandom;
    static boost::taus88 gen(urandom);
    boost::uniform_real<> dist(-jitter/2, jitter/2);
    boost::variate_generator<boost::taus88&, boost::uniform_real<> >
            random(gen, dist); 
    std::vector<Segment> result;
    std::string line;
    while(getline(fi, line)) {
        auto numbers = parse_numbers(line);
        if(jitter)
            for(auto & n : numbers) n += random();
        if(numbers.size() % 2 != 0)
            throw std::invalid_argument("odd number of values in segment line");
        if(numbers.size() < 6)
            throw std::invalid_argument("too few values in segment line");
        for(size_t i=0; i<numbers.size() - 3 ; i += 2) {
            Segment s{{numbers[i], numbers[i+1]}, {numbers[i+2], numbers[i+3]}};
            result.push_back(s);
        }
        int i = numbers.size();
        Segment s{{numbers[0], numbers[1]}, {numbers[i-2], numbers[i-1]}};
        result.push_back(s);
    }
    return result;
}

std::vector<Segment> SegmentsFromFile(const char *filename, double jitter) {
    std::fstream fi(filename);
    return SegmentsFromFile(fi, jitter);
}

void usage(const char *argv0) {
    fprintf(stderr,
        "Usage: %s [-b] [-s scale] [-j jitter] patfile segfile\n",
        argv0);
    exit(1);
}


int main(int argc, char **argv) {
    auto scale = 1., jitter = 0.;
    bool bench = false;
    int c;
    while((c = getopt(argc, argv, "bs:j:")) > 0) {
        switch(c) {
        case 'b': bench = !bench; break;
        case 's': scale = atof(optarg); break;
        case 'j': jitter = atof(optarg); break;
        default:
            usage(argv[0]);
        }
    }
    auto nargs = argc - optind;
    if(nargs != 2) usage(argv[0]);

    auto patfile = argv[optind];
    auto segfile = argv[optind+1];

    auto h = HatchPattern::FromFile(patfile, scale);
    auto s = SegmentsFromFile(segfile, jitter);

    if(bench) {
        int nseg = 0;
        auto print_seg = [&nseg](const Segment &s) { (void)s; nseg ++; };
        xyhatch(h, s, print_seg);
        std::cout << nseg << "\n";
        return 0;
    }

    auto cmp_x = [](const Segment &a, const Segment & b)
            { return a.p.x < b.p.x; };
    auto cmp_y = [](const Segment &a, const Segment & b)
            { return a.p.y < b.p.y; };
    auto min_x = std::min_element(s.begin(), s.end(), cmp_x)->p.x;
    auto max_x = std::max_element(s.begin(), s.end(), cmp_x)->p.x;
    auto min_y = -std::max_element(s.begin(), s.end(), cmp_y)->p.y;
    auto max_y = -std::min_element(s.begin(), s.end(), cmp_y)->p.y;
    auto d_x = max_x - min_x;
    auto d_y = max_y - min_y;

    std::cout <<
        "<svg width=\"100%%\" height=\"100%%\" viewBox=\""
            << (min_x - .05 * d_x) << " " << (min_y - .05 * d_x) << " "
            << (d_x * 1.1)         << " " << (d_y * 1.1) << "\" "
      "preserveAspectRatio=\"xMidyMid\" "
      "xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" "
      "xmlns:xlink=\"http://www.w3.org/1999/xlink\">"
      "<path d=\"M-100000 0L100000 0M0 -100000L0 100000\" stroke=\"green\" "
      "stroke-dasharray=\"20 20\"/>";

    auto print_seg = [](const Segment &s) {
        std::cout << "M" << s.p.x << " " << -s.p.y
                  << "L" << s.q.x << " " << -s.q.y << "\n";
    };

    std::cout << "<path fill=\"none\" stroke=\"black\" "
                 "stroke-linecap=\"round\"  d=\"";
    for(const auto i : s) print_seg(i);
    std::cout << "\"/>";

    std::vector<Segment> segs;
    std::cout << "<path fill=\"none\" stroke=\"blue\" "
                 "stroke-linecap=\"round\"  d=\"";
    xyhatch(h, s, print_seg);
    std::cout << "\"/>";

    std::cout << "</svg>";
    return 0;
}
