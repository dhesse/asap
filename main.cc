#include <flight.hpp>

using namespace asap;

int main() {
  Flight f("sample_flight.asc");
  f.checkin(TravelCategory::kEconomy, "Ben", false, "6A");
  PassengerGroup g(TravelCategory::kEconomy);
  g.push("Kate", SeatType::kWindow, false);
  g.push("Jack", SeatType::kAisle, false);
  g.push("Hugo", SeatType::kOther, false);
  g.push("James", SeatType::kWindow, false);
  f.checkin(g);
  PassengerGroup a("passengers1.asc");
  PassengerGroup b("passengers2.asc");
  f.checkin(a);
  f.checkin(b);
  f.checkin(TravelCategory::kEconomy, "Boone", SeatType::kWindow);
  f.show();
}
