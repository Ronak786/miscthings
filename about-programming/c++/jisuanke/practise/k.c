for (auto i = first; i != prev(last); ++i){
    for (auto j = prev(last); j != i; --j) {
        auto k = prev(j);
        if (*j < *k) {
            auto t = *j;
            *j = *k;
            *k = t;
        }
    }
}

