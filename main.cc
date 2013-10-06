#include <flight.hpp>

int main() {
  asap::Flight f("sample_flight.asc");
  //f.show();
  asap::PassengerGroup g(asap::TravelCategory::kEconomy);
  g.push("John", asap::SeatType::kWindow, false);
  g.push("Paul", asap::SeatType::kAisle, false);
  g.push("George", asap::SeatType::kOther, false);
  g.push("Ringo", asap::SeatType::kWindow, false);
  f.checkin(g);
  asap::PassengerGroup a("passengers1.asc");
  asap::PassengerGroup b("passengers2.asc");
  f.checkin(a);
  f.checkin(b);
  f.show();
}
