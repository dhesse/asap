#include <flight.hpp>

int main() {
  asap::Flight f("sample_flight.asc");
  f.checkin(asap::TravelCategory::kEconomy, "Ben", false, "6A");
  asap::PassengerGroup g(asap::TravelCategory::kEconomy);
  g.push("Kate", asap::SeatType::kWindow, false);
  g.push("Jack", asap::SeatType::kAisle, false);
  g.push("Hugo", asap::SeatType::kOther, false);
  g.push("James", asap::SeatType::kWindow, false);
  f.checkin(g);
  asap::PassengerGroup a("passengers1.asc");
  asap::PassengerGroup b("passengers2.asc");
  f.checkin(a);
  f.checkin(b);
  f.show();
}
