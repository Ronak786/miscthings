#include "weather.h"

namespace webdata {
WebData::WebData(double a, double b, double c) :
    one_(a), two_(b), three_(c) {}

void WebData::RegisterObserver(Observer* obptr) {
    oblist_.push_front(obptr);
}

void WebData::RemoveObserver(Observer* obptr) {
    oblist_.remove(obptr);
}

void WebData::NotifyObserver() {
    for (auto i : oblist_)
        i->update();
}

void Observer::Subscribe(Subject *subptr) {
    subptr_ = subptr;
    subptr->RegisterObserver(this);
}

} //namespace webdata

int main() {
    webdata::WebData web(1,2,3);
    webdata::CurrentConditionDisplay curr;
    webdata::StatisticsDisplay stat;
    webdata::ForecastDisplay fore;
    curr.Subscribe(&web);
    stat.Subscribe(&web);
    fore.Subscribe(&web);
    web.NotifyObserver();
    return 0;
}
