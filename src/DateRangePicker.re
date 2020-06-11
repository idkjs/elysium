open MomentRe;
type dates = {
  startDate: MomentRe.Moment.t,
  endDate: MomentRe.Moment.t,
};
type onDatesChangeType = dates => unit;

[@bs.module "./DateRangePicker.r.js"] [@react.component]
external make: (~startDate: Moment.t, ~endDate: Moment.t, ~onDatesChange: onDatesChangeType) => React.element =
  "default";
