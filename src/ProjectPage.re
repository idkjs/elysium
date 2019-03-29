module Styles = {
  open Css;
  let project =
    style([
      display(`flex),
      flexDirection(column),
      alignItems(center),
      justifyContent(center),
      minWidth(vw(90.0)),
      color(hex("232c33")),
    ]);
};
let component = ReasonReact.statelessComponent("ProjectPage");

open Types;

module GetProjectBySlug = [%graphql
  {|
 query getProjectBySlug($slug: String!) {
    projectBySlug(slug: $slug) {
      id
      name
      slug
      color
      description
      contributors {
        id
        displayName
      }
    }
 }
|}
];

module GetProjectBySlugQuery = ReasonApollo.CreateQuery(GetProjectBySlug);

let make = (_children, ~slug) => {
  ...component,
  render: _self => {
    let slugQuery = GetProjectBySlug.make(~slug, ());
    <GetProjectBySlugQuery variables=slugQuery##variables>
      ...{({result}) =>
        switch (result) {
        | Loading => <div> {ReasonReact.string("Loading")} </div>
        | Error(error) => <div> {ReasonReact.string(error##message)} </div>
        | Data(response) =>
          let project = projectFromJs(response##projectBySlug);
          let contributors = response##projectBySlug##contributors |> Array.map(user => userFromJs(user));
          <div className=Styles.project>
            <h1> {ReasonReact.string(project.name)} </h1>
            <p>
              {switch (project.description) {
               | None => ReasonReact.string("No description")
               | Some(desc) => ReasonReact.string(desc)
               }}
            </p>
            <h2> {ReasonReact.string("Contributors")} </h2>
            <div>
              ...{
                   contributors
                   |> Array.map((c: user) =>
                        <div key={string_of_int(c.id)}> {ReasonReact.string(c.displayName)} </div>
                      )
                 }
            </div>
          </div>;
        }
      }
    </GetProjectBySlugQuery>;
  },
};
