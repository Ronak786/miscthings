#include "weather.h"

namespace webdata {
WebData::WebData(double a, double b, double c) :
    one_(a), two_(b), three_(c) {}

void WebData::RegisterObserver(const Observer* obptr) {
    oblist_.push_front(obptr);
}

void WebData::RemoveObserver(const Observer* obptr) {
    oblist_.remove(obptr);
}

void WebData::NotifyObserver() {
    for (auto i : oblist_)
        i->update();
}

void Observer::Subscribe(Subject *subptr) {
    subptr_ = subptr;
    subptr->RemoveObserver(this);
}

} //namespace webdata
