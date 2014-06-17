/*
 * Copyright 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Charles Kerr <charles.kerr@canonical.com>
 */

#include <transfer/dbus-shared.h>
#include <transfer/controller.h>
#include <transfer/view-gmenu.h>

#include <core/connection.h>

#include <glib/gi18n.h>
#include <gio/gio.h>

#include <algorithm> // std::sort()

namespace unity {
namespace indicator {
namespace transfer {

/****
*****
****/

namespace {

/**
 * \brief GActionGroup wrapper that routes action callbacks to the Controller
 */
class GActions
{
public:

  GActions(const std::shared_ptr<Model>& model,
           const std::shared_ptr<Controller>& controller):
    m_action_group(g_simple_action_group_new()),
    m_controller(controller)
  {
    set_model(model);

    const GActionEntry entries[] = {
        { "activate-transfer", on_tap, "s", nullptr },
        { "cancel-transfer", on_cancel, "s", nullptr },
        { "pause-transfer", on_pause, "s", nullptr },
        { "resume-transfer", on_resume, "s", nullptr },
        { "open-transfer", on_open, "s", nullptr },
        { "open-app-transfer", on_open_app, "s", nullptr },
        { "resume-all", on_resume_all },
        { "pause-all", on_pause_all },
        { "clear-all", on_clear_all }
    };

    auto gam = G_ACTION_MAP(m_action_group);
    g_action_map_add_action_entries(gam,
                                    entries,
                                    G_N_ELEMENTS(entries),
                                    this);

    // add the header actions
    auto v = create_default_header_state();
    auto a = g_simple_action_new_stateful("phone-header", nullptr, v);
    g_action_map_add_action(gam, G_ACTION(a));

    // add the transfer-states dictionary
    v = create_transfer_states();
    a = g_simple_action_new_stateful("transfer-states", nullptr, v);
    g_action_map_add_action(gam, G_ACTION(a));
  }

  void set_model(const std::shared_ptr<Model>& model)
  {
    auto& c = m_connections;
    c.clear();

    if ((m_model = model))
      {
        auto updater = [this](const Transfer::Id&) {update_soon();};
        c.insert(m_model->added().connect(updater));
        c.insert(m_model->changed().connect(updater));
        c.insert(m_model->removed().connect(updater));
        update_soon();
      }
  }

  ~GActions()
  {
    if (m_update_tag)
      g_source_remove (m_update_tag);
    g_clear_object(&m_action_group);
  }

  GActionGroup* action_group() const
  {
    return G_ACTION_GROUP(m_action_group);
  }

private:

  /***
  ****  TRANSFER STATES
  ***/

  void update_soon()
  {
    if (m_update_tag == 0)
      m_update_tag = g_timeout_add_seconds(1, update_timeout, this);
  }

  static gboolean update_timeout(gpointer gself)
  {
    auto self = static_cast<GActions*>(gself);
    self->m_update_tag = 0;
    self->update();
    return G_SOURCE_REMOVE;
  }

  void update()
  {
    g_action_group_change_action_state(action_group(),
                                       "transfer-states",
                                       create_transfer_states());
  }

  GVariant* create_transfer_states()
  {
    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE_VARDICT);

    for (const auto& transfer : m_model->get_all())
      {
        auto state = create_transfer_state(transfer);
        g_variant_builder_add(&b, "{sv}", transfer->id.c_str(), state);
      }

    return g_variant_builder_end(&b);
  }

  GVariant* create_transfer_state(const std::shared_ptr<Transfer>& transfer)
  {
    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE_VARDICT);

    g_variant_builder_add(&b, "{sv}", "percent",
                          g_variant_new_double(transfer->progress));
    if (transfer->seconds_left >= 0)
      {
        g_variant_builder_add(&b, "{sv}", "seconds-left",
                              g_variant_new_int32(transfer->seconds_left));
      }

    g_variant_builder_add(&b, "{sv}", "state", g_variant_new_int32(transfer->state));

    return g_variant_builder_end(&b);
  }

  /***
  ****  ACTION CALLBACKS
  ***/

  std::shared_ptr<Controller>& controller()
  {
    return m_controller;
  }

  static void on_tap(GSimpleAction*, GVariant* vuid, gpointer gself)
  {
    const auto uid = g_variant_get_string(vuid, nullptr);
    static_cast<GActions*>(gself)->controller()->tap(uid);
  }

  static void on_cancel(GSimpleAction*, GVariant* vuid, gpointer gself)
  {
    const auto uid = g_variant_get_string(vuid, nullptr);
    static_cast<GActions*>(gself)->controller()->cancel(uid);
  }

  static void on_pause(GSimpleAction*, GVariant* vuid, gpointer gself)
  {
    const auto uid = g_variant_get_string(vuid, nullptr);
    static_cast<GActions*>(gself)->controller()->pause(uid);
  }

  static void on_resume(GSimpleAction*, GVariant* vuid, gpointer gself)
  {
    const auto uid = g_variant_get_string(vuid, nullptr);
    static_cast<GActions*>(gself)->controller()->resume(uid);
  }

  static void on_open(GSimpleAction*, GVariant* vuid, gpointer gself)
  {
    const auto uid = g_variant_get_string(vuid, nullptr);
    static_cast<GActions*>(gself)->controller()->open(uid);
  }

  static void on_open_app(GSimpleAction*, GVariant* vuid, gpointer gself)
  {
    const auto uid = g_variant_get_string(vuid, nullptr);
    static_cast<GActions*>(gself)->controller()->open_app(uid);
  }

  static void on_resume_all(GSimpleAction*, GVariant*, gpointer gself)
  {
    static_cast<GActions*>(gself)->controller()->resume_all();
  }

  static void on_clear_all(GSimpleAction*, GVariant*, gpointer gself)
  {
    static_cast<GActions*>(gself)->controller()->clear_all();
  }

  static void on_pause_all(GSimpleAction*, GVariant*, gpointer gself)
  {
    static_cast<GActions*>(gself)->controller()->pause_all();
  }

  GVariant* create_default_header_state()
  {
    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add(&b, "{sv}", "accessible-desc",
                          g_variant_new_string("accessible-desc"));
    g_variant_builder_add(&b, "{sv}", "label", g_variant_new_string("label"));
    g_variant_builder_add(&b, "{sv}", "title", g_variant_new_string("title"));
    g_variant_builder_add(&b, "{sv}", "visible", g_variant_new_boolean(true));
    return g_variant_builder_end(&b);
  }

  /***
  ****
  ***/

  GSimpleActionGroup* m_action_group = nullptr;
  std::shared_ptr<Model> m_model;
  std::shared_ptr<Controller> m_controller;
  std::set<core::ScopedConnection> m_connections;
  guint m_update_tag = 0;

  // we've got raw pointers in here, so disable copying
  GActions(const GActions&) =delete;
  GActions& operator=(const GActions&) =delete;
};

/***
****
***/

/**
 * \brief A menu for a specific profile; eg, Desktop or Phone.
 */
class Menu
{
public:

  enum Profile { DESKTOP, PHONE, NUM_PROFILES };
  enum Section { ONGOING, SUCCESSFUL, NUM_SECTIONS };

  const char* name() const { return m_name; }
  GMenuModel* menu_model() { return G_MENU_MODEL(m_menu); }

  Menu(const char* name_in,
       const std::shared_ptr<Model>& model,
       const std::shared_ptr<GActions>& gactions):
    m_name{name_in},
    m_gactions{gactions}
  {
    // initialize the menu
    create_gmenu();
    set_model(model);
    update_section(ONGOING);
    update_section(SUCCESSFUL);
  }

  virtual ~Menu()
  {
    if (m_update_menu_tag)
      g_source_remove(m_update_menu_tag);
    if (m_update_header_tag)
      g_source_remove(m_update_header_tag);
    g_clear_object(&m_menu);
  }

  void set_model (const std::shared_ptr<Model>& model)
  {
    auto& c = m_connections;
    c.clear();

    if ((m_model = model))
      {
        auto updater = [this](const Transfer::Id&) {
          update_header_soon();
          update_menu_soon();
        };
        c.insert(m_model->added().connect(updater));
        c.insert(m_model->changed().connect(updater));
        c.insert(m_model->removed().connect(updater));
      }

    update_header();
  }

private:

  void create_gmenu()
  {
    g_assert(m_submenu == nullptr);

    m_submenu = g_menu_new();

    // build placeholders for the sections
    for(int i=0; i<NUM_SECTIONS; i++)
    {
      auto item = g_menu_item_new(nullptr, nullptr);
      g_menu_append_item(m_submenu, item);
      g_object_unref(item);
    }

    // add submenu to the header
    const auto detailed_action = (std::string{"indicator."} + name()) + "-header";
    auto header = g_menu_item_new(nullptr, detailed_action.c_str());
    g_menu_item_set_attribute(header, "x-canonical-type", "s",
                              "com.canonical.indicator.root");
    g_menu_item_set_submenu(header, G_MENU_MODEL(m_submenu));
    g_object_unref(m_submenu);

    // add header to the menu
    m_menu = g_menu_new();
    g_menu_append_item(m_menu, header);
    g_object_unref(header);
  }

  /***
  ****  HEADER
  ***/

  void update_header_soon()
  {
    if (m_update_header_tag == 0)
      m_update_header_tag = g_timeout_add(100, update_header_now, this);
  }
  static gboolean update_header_now (gpointer gself)
  {
    auto* self = static_cast<Menu*>(gself);
    self->m_update_header_tag = 0;
    self->update_header();
    return G_SOURCE_REMOVE;
  }

  void update_header()
  {
    auto action_name = g_strdup_printf("%s-header", name());
    auto state = create_header_state();
    g_action_group_change_action_state(m_gactions->action_group(), action_name, state);
    g_free(action_name);
  }

  // FIXME: see fixme comment for create_header_label()
  GVariant* create_header_icon()
  {
    return create_image_missing_icon();
  }

  // FIXME: this information is supposed to be given the user via an icon.
  // since the icons haven't been drawn yet, use a placeholder label instead.
  GVariant* create_header_label() const
  {
    int n_in_progress = 0;
    int n_failed = 0;
    int n_paused = 0;

    for (const auto& transfer : m_model->get_all())
      {
        switch (transfer->state)
          {
            case Transfer::RUNNING:
            case Transfer::HASHING:
            case Transfer::PROCESSING:
              ++n_in_progress;
              break;

            case Transfer::PAUSED:
              ++n_paused;
              break;

            case Transfer::ERROR:
              ++n_failed;
              break;

            case Transfer::QUEUED:
            case Transfer::CANCELED:
            case Transfer::FINISHED:
              break;
          }
      }

    char* str;
    if (n_in_progress > 0)
      str = g_strdup_printf ("%d active", n_in_progress);
    else if (n_paused > 0)
      str = g_strdup_printf ("%d paused", n_paused);
    else if (n_failed > 0)
      str = g_strdup_printf ("%d failed", n_failed);
    else
      str = g_strdup_printf ("idle");

    return g_variant_new_take_string(str);
  }

  GVariant* create_header_state()
  {
    auto title_v = g_variant_new_string(_("Transfers"));

    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add(&b, "{sv}", "title", title_v);
    g_variant_builder_add(&b, "{sv}", "icon", create_header_icon());
    g_variant_builder_add(&b, "{sv}", "label", create_header_label());
    g_variant_builder_add(&b, "{sv}", "accessible-desc", title_v);
    g_variant_builder_add(&b, "{sv}", "visible", g_variant_new_boolean(true));
    return g_variant_builder_end (&b);
  }

  /***
  ****  MENU
  ***/

  void update_menu_soon()
  {
    if (m_update_menu_tag == 0)
      m_update_menu_tag = g_timeout_add_seconds (1, update_menu_now, this);
  }
  static gboolean update_menu_now (gpointer gself)
  {
    auto* self = static_cast<Menu*>(gself);
    self->m_update_menu_tag = 0;
    self->update_section(ONGOING);
    self->update_section(SUCCESSFUL);
    return G_SOURCE_REMOVE;
  }

  void update_section(Section section)
  {
    GMenuModel * model;

    switch (section)
      {
        case ONGOING:
          model = create_ongoing_transfers_section();
          break;

        case SUCCESSFUL:
          model = create_successful_transfers_section();
          break;

        case NUM_SECTIONS:
          model = nullptr;
          g_warn_if_reached();
      }

    if (model)
      {
        g_menu_remove(m_submenu, section);
        g_menu_insert_section(m_submenu, section, nullptr, model);
        g_object_unref(model);
      }
  }

  GMenuModel* create_ongoing_transfers_section()
  {
    auto menu = g_menu_new();

    // build a list of the ongoing transfers
    std::vector<std::shared_ptr<Transfer>> transfers;
    for (const auto& transfer : m_model->get_all())
      if (transfer->state != Transfer::FINISHED)
        transfers.push_back (transfer);

    // soft them in reverse chronological order s.t.
    // the most recent transfer is displayed first
    auto compare = [](const std::shared_ptr<Transfer>& a,
                      const std::shared_ptr<Transfer>& b){
      return a->time_started > b->time_started;
    };
    std::sort(transfers.begin(), transfers.end(), compare);

    // add the bulk actions menuitem ("Resume all" or "Pause all")
    int n_can_resume = 0;
    for (const auto& t : transfers)
      if (t->can_resume())
        ++n_can_resume;
    GMenuItem* menu_item;
    if (n_can_resume > 0)
      menu_item = g_menu_item_new(_("Resume all"), "indicator.resume-all");
    else
      menu_item = g_menu_item_new(_("Pause all"), "indicator.pause-all");
    g_menu_append_item(menu, menu_item);
    g_object_unref(menu_item);

    // add the transfers
    for (const auto& t : transfers)
      append_transfer_menuitem(menu, t);

    return G_MENU_MODEL(menu);
  }

  GMenuModel* create_successful_transfers_section()
  {
    auto menu = g_menu_new();

    // build a list of the successful transfers
    std::vector<std::shared_ptr<Transfer>> transfers;
    for (const auto& transfer : m_model->get_all())
      if (transfer->state == Transfer::FINISHED)
        transfers.push_back (transfer);

    // as per spec, sort s.t. most recent transfer is first
    auto compare = [](const std::shared_ptr<Transfer>& a,
                      const std::shared_ptr<Transfer>& b){
      return a->time_started > b->time_started;
    };
    std::sort(transfers.begin(), transfers.end(), compare);

    // as per spec, limit the list to 10 items
    constexpr int max_items = 10;
    if (transfers.size() > max_items)
      transfers.erase(transfers.begin()+max_items, transfers.end());

    // add the bulk actions menuitem ("Clear all" or "Pause all")
    auto menu_item = g_menu_item_new(_("Clear all"), "indicator.clear-all");
    g_menu_append_item(menu, menu_item);
    g_object_unref(menu_item);

    // add the transfers
    for (const auto& t : transfers)
      append_transfer_menuitem(menu, t);

    return G_MENU_MODEL(menu);
  }

  void append_transfer_menuitem(GMenu* menu,
                                const std::shared_ptr<Transfer>& transfer)
  {
    auto menuitem = create_transfer_menuitem(transfer);
    g_menu_append_item(menu, menuitem);
    g_object_unref(menuitem);
  }

  GMenuItem* create_transfer_menuitem(const std::shared_ptr<Transfer>& transfer)
  {
    const auto& id = transfer->id.c_str();

    GMenuItem* menu_item;

    if (!transfer->title.empty())
      {
        menu_item = g_menu_item_new (transfer->title.c_str(), nullptr);
      }
    else
      {
        char* size = g_format_size (transfer->total_size);
        char* label = g_strdup_printf(_("Unknown Download (%s)"), size);
        menu_item = g_menu_item_new (label, nullptr);
        g_free(label);
        g_free(size);
      }

    g_menu_item_set_attribute (menu_item, "x-canonical-type",
                               "s", "com.canonical.indicator.transfer");
    g_menu_item_set_attribute_value (menu_item, G_MENU_ATTRIBUTE_ICON,
                                     create_transfer_icon(transfer));
    g_menu_item_set_attribute (menu_item, "x-canonical-uid", "s", id);
    g_menu_item_set_action_and_target_value (menu_item,
                                             "indicator.activate-transfer",
                                             g_variant_new_string(id));
    return G_MENU_ITEM(menu_item);
  }

  GVariant* create_transfer_icon(const std::shared_ptr<Transfer>&)// transfer)
  {
    //FIXME: this is a placeholder
    return create_image_missing_icon();
  }

  GVariant* create_image_missing_icon() // FIXME: this is a placeholder
  {
    auto icon = g_themed_icon_new("image-missing");
    auto v = g_icon_serialize(icon);
    g_object_unref(icon);
    return v;
  }

  /***
  ****
  ***/

  std::set<core::ScopedConnection> m_connections;
  GMenu* m_menu = nullptr;
  const char* const m_name;

  std::shared_ptr<Model> m_model;
  std::shared_ptr<GActions> m_gactions;
  GMenu* m_submenu = nullptr;

  guint m_update_menu_tag = 0;
  guint m_update_header_tag = 0;

  // we've got raw pointers in here, so disable copying
  Menu(const Menu&) =delete;
  Menu& operator=(const Menu&) =delete;
};

/***
****
***/

/**
 * \brief Exports actions and gmenus to the DBus
 */
class Exporter
{
public:

  Exporter(){}

  ~Exporter()
  {
    if (m_bus != nullptr)
      {
        for(const auto& id : m_exported_menu_ids)
          g_dbus_connection_unexport_menu_model(m_bus, id);

        if (m_exported_actions_id)
          g_dbus_connection_unexport_action_group(m_bus,
                                                  m_exported_actions_id);
    }

    if (m_own_id)
      g_bus_unown_name(m_own_id);

    g_clear_object(&m_bus);
  }

  core::Signal<> name_lost;

  void publish(const std::shared_ptr<GActions>& gactions,
               const std::vector<std::shared_ptr<Menu>>& menus)
  {
    m_gactions = gactions;
    m_menus = menus;
    m_own_id = g_bus_own_name(G_BUS_TYPE_SESSION,
                              BUS_NAME,
                              G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT,
                              on_bus_acquired,
                              nullptr,
                              on_name_lost,
                              this,
                              nullptr);
  }

private:

  /***
  ****
  ***/

  static void on_bus_acquired(GDBusConnection* connection,
                                        const gchar* name,
                                        gpointer gself)
  {
    g_debug("bus acquired: %s", name);
    static_cast<Exporter*>(gself)->on_bus_acquired(connection, name);
  }

  void on_bus_acquired(GDBusConnection* connection, const gchar* /*name*/)
  {
    m_bus = G_DBUS_CONNECTION(g_object_ref(G_OBJECT(connection)));

    // export the actions
    GError * error = nullptr;
    auto id = g_dbus_connection_export_action_group(m_bus,
                                                    BUS_PATH,
                                                    m_gactions->action_group(),
                                                    &error);
    if (id)
      {
        m_exported_actions_id = id;
      }
    else
      {
        g_warning("cannot export action group: %s", error->message);
        g_clear_error(&error);
      }

    // export the menus
    for(auto& menu : m_menus)
      {
        const auto path = std::string(BUS_PATH) + "/" + menu->name();
        id = g_dbus_connection_export_menu_model(m_bus,
                                                 path.c_str(),
                                                 menu->menu_model(),
                                                 &error);
        if (id)
          {
            m_exported_menu_ids.insert(id);
          }
        else
          {
            if (error != nullptr)
                g_warning("cannot export %s menu: %s", menu->name(), error->message);

            g_clear_error(&error);
          }
      }
  }

  /***
  ****
  ***/

  static void on_name_lost(GDBusConnection* connection,
                           const gchar* name,
                           gpointer gthis)
  {
    g_debug("name lost: %s", name);
    static_cast<Exporter*>(gthis)->on_name_lost(connection, name);
  }

  void on_name_lost(GDBusConnection* /*connection*/, const gchar* /*name*/)
  {
    name_lost();
  }

  /***
  ****
  ***/

  std::set<guint> m_exported_menu_ids;
  guint m_own_id = 0;
  guint m_exported_actions_id = 0;
  GDBusConnection * m_bus = nullptr;
  std::shared_ptr<GActions> m_gactions;
  std::vector<std::shared_ptr<Menu>> m_menus;

  // we've got raw pointers and gsignal tags in here, so disable copying
  Exporter(const Exporter&) =delete;
  Exporter& operator=(const Exporter&) =delete;
};

} // anonymous namespace

/***
****
***/

class GMenuView::Impl
{
public:

  Impl (const std::shared_ptr<Model>& model,
        const std::shared_ptr<Controller>& controller):
    m_model(model),
    m_controller(controller),
    m_gactions(new GActions(model, controller)),
    m_exporter(new Exporter)
  {
    // create the Menus
    for(int i=0; i<Menu::NUM_PROFILES; i++)
      m_menus.push_back(create_menu_for_profile(Menu::Profile(i)));
  
    m_exporter->publish(m_gactions, m_menus);
  }

  ~Impl()
  {
  }

  void set_model(const std::shared_ptr<Model>& model)
  {
    m_model = model;

    for(const auto& menu : m_menus)
      menu->set_model(model);
  }

  const core::Signal<>& name_lost() { return m_exporter->name_lost; }

private:

  std::shared_ptr<Menu> create_menu_for_profile(Menu::Profile profile)
  {
    // only one design, so for now everything uses the phone menu
    constexpr static const char* profile_names[] = { "desktop", "phone" };
    std::shared_ptr<Menu> m(new Menu(profile_names[profile],
                                     m_model,
                                     m_gactions));
    return m;
  }

  std::shared_ptr<Model> m_model;
  std::shared_ptr<Controller> m_controller;
  std::shared_ptr<GActions> m_gactions;
  std::vector<std::shared_ptr<Menu>> m_menus;
  std::shared_ptr<Exporter> m_exporter;
};

/***
****
***/

GMenuView::GMenuView(const std::shared_ptr<Model>& model,
                 const std::shared_ptr<Controller>& controller):
  p(new Impl(model, controller))
{
}

GMenuView::~GMenuView()
{
}

void GMenuView::set_model(const std::shared_ptr<Model>& model)
{
  p->set_model(model);
}

const core::Signal<>& GMenuView::name_lost() const
{
  return p->name_lost();
}

/***
****
***/

} // namespace transfer
} // namespace indicator
} // namespace unity

