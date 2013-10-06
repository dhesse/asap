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
  f.show();
  g.empty();
  g.push("Joey", asap::SeatType::kWindow, false);
  g.push("Johnny", asap::SeatType::kAisle, true);
  g.push("Dee", asap::SeatType::kOther, false);
  f.checkin(g);
  f.show();
}
