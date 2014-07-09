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

  }

  void set_model(const std::shared_ptr<Model>& model)
  {
    // out with the old...
    auto& c = m_connections;
    c.clear();
    if (m_model)
        for (const auto& id : m_model->get_ids())
          remove(id);

    // ...in with the new
    if ((m_model = model))
      {
        c.insert(m_model->added().connect([this](const Transfer::Id& id){add(id);}));
        c.insert(m_model->changed().connect([this](const Transfer::Id& id){update(id);}));
        c.insert(m_model->removed().connect([this](const Transfer::Id& id){remove(id);}));

        // add the transfers
        for (const auto& id : m_model->get_ids())
          add(id);
      }
  }

  ~GActions()
  {
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

  void add(const Transfer::Id& id)
  {
    const auto name = get_transfer_action_name(id);
    const auto state = create_transfer_state(id);
    auto a = g_simple_action_new_stateful(name.c_str(), nullptr, state);
    g_action_map_add_action(action_map(), G_ACTION(a));
  }

  void update(const Transfer::Id& id)
  {
    const auto name = get_transfer_action_name(id);
    const auto state = create_transfer_state(id);
    g_action_group_change_action_state(action_group(), name.c_str(), state);
  }

  void remove(const Transfer::Id& id)
  {
    const auto name = get_transfer_action_name(id);
    g_action_map_remove_action(action_map(), name.c_str());
  }

  std::string get_transfer_action_name (const Transfer::Id& id)
  {
    return std::string("transfer-state.") + id;
  }

  GVariant* create_transfer_state(const Transfer::Id& id)
  {
    return create_transfer_state(m_model->get(id));
  }

  GVariant* create_transfer_state(const std::shared_ptr<Transfer>& transfer)
  {
    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE_VARDICT);

    if (transfer)
      {
        g_variant_builder_add(&b, "{sv}", "percent",
                              g_variant_new_double(transfer->progress));
        if (transfer->seconds_left >= 0)
          {
            g_variant_builder_add(&b, "{sv}", "seconds-left",
                                  g_variant_new_int32(transfer->seconds_left));
          }

        g_variant_builder_add(&b, "{sv}", "state", g_variant_new_int32(transfer->state));
      }
    else
      {
        g_warn_if_reached();
      }

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

  GActionMap* action_map() const
  {
    return G_ACTION_MAP(m_action_group);
  }

  GSimpleActionGroup* m_action_group = nullptr;
  std::shared_ptr<Model> m_model;
  std::shared_ptr<Controller> m_controller;
  std::set<core::ScopedConnection> m_connections;

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
  }

  virtual ~Menu()
  {
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
        c.insert(m_model->added().connect([this](const Transfer::Id& id){add(id);}));
        c.insert(m_model->changed().connect([this](const Transfer::Id& id){update(id);}));
        c.insert(m_model->removed().connect([this](const Transfer::Id& id){remove(id);}));

        // add the transfers
        for (const auto& id : m_model->get_ids())
          add(id);
      }

    update_header();
  }

private:

  void create_gmenu()
  {
    g_assert(m_submenu == nullptr);

    // build the submenu
    m_submenu = g_menu_new();
    for(int i=0; i<NUM_SECTIONS; i++)
      {
        auto section = g_menu_new();
        g_menu_append_section(m_submenu, nullptr, G_MENU_MODEL(section));
        g_object_unref(section);
      }

    // add submenu to the header
    const auto detailed_action = (std::string{"indicator."} + name()) + "-header";
    auto header = g_menu_item_new(nullptr, detailed_action.c_str());
    g_menu_item_set_attribute(header, ATTRIBUTE_X_TYPE, "s",
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

  GVariant* get_header_icon() const
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

    const char * name;
    if (n_in_progress > 0)
      name = "transfer-progress";
    else if (n_paused > 0)
      name = "transfer-paused";
    else if (n_failed > 0)
      name = "transfer-error";
    else
      name = "transfer-none";

    auto icon = g_themed_icon_new_with_default_fallbacks(name);
    auto ret = g_icon_serialize(icon);
    g_object_unref(icon);

    return ret;
  }

  GVariant* create_header_state()
  {
    auto reffed_icon_v = get_header_icon();
    auto title_v = g_variant_new_string(_("Transfers"));

    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add(&b, "{sv}", "title", title_v);
    g_variant_builder_add(&b, "{sv}", "icon", reffed_icon_v);
    g_variant_builder_add(&b, "{sv}", "accessible-desc", title_v);
    g_variant_builder_add(&b, "{sv}", "visible", g_variant_new_boolean(true));
    auto ret = g_variant_builder_end (&b);

    g_variant_unref(reffed_icon_v);
    return ret;
  }

  /***
  ****  MENU
  ***/

  GMenuItem* create_bulk_action_menuitem(const char* label,
                                         const char* extra_label,
                                         const char* detailed_action)
  {
    auto item = g_menu_item_new(label, detailed_action);

    g_menu_item_set_attribute(item, ATTRIBUTE_X_TYPE, "s",
                              "com.canonical.indicator.button-section");

    if (extra_label && *extra_label)
      {
        g_menu_item_set_attribute(item, ATTRIBUTE_X_LABEL,
                                  "s", extra_label);
      }

    return item;
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

    g_menu_item_set_attribute (menu_item, ATTRIBUTE_X_TYPE,
                               "s", "com.canonical.indicator.transfer");
    g_menu_item_set_attribute_value (menu_item, G_MENU_ATTRIBUTE_ICON,
                                     create_transfer_icon(transfer));
    g_menu_item_set_attribute (menu_item, ATTRIBUTE_X_UID, "s", id);
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

  static bool menuitem_attribute_equal(GMenuModel* model,
                                       int item_index,
                                       GMenuItem* candidate,
                                       const char* attribute)
  {
    auto a = g_menu_model_get_item_attribute_value(model, item_index, attribute, nullptr);
    auto b = g_menu_item_get_attribute_value(candidate, attribute, nullptr);
    const bool equal = g_variant_equal(a, b);
    g_clear_pointer(&a, g_variant_unref);
    g_clear_pointer(&b, g_variant_unref);
    return equal;
  }

  bool transfer_menuitem_equal(GMenuModel* model,
                               int item_index,
                               GMenuItem* candidate) const
  {
    return menuitem_attribute_equal(model, item_index, candidate, G_MENU_ATTRIBUTE_LABEL)
        && menuitem_attribute_equal(model, item_index, candidate, G_MENU_ATTRIBUTE_ICON);
  }

  static Section get_transfer_section(const std::shared_ptr<Transfer>& transfer)
  {
    return (transfer->state == Transfer::FINISHED) ? SUCCESSFUL : ONGOING;
  }

#if 0
  GMenu* find_gmenu_for_section (Section section) const
  {
    auto mm = G_MENU_MODEL(m_submenu);

    return g_menu_model_get_n_items(mm) >= section
      ? G_MENU(g_menu_model_get_item_link(mm, section, G_MENU_LINK_SECTION))
      : nullptr;
  }
#endif

#if 0
  bool find_current_menuitem (const Transfer::Id& id, Section& section, GMenu*& gmenu, int& pos) const
  {
    // find the section
    const auto it = m_visible_transfers.find(id);
    if (it == m_visible_transfers.end())
      return false;
    section = it->second;

    // find the menu
    gmenu = find_gmenu_for_section(section);
    if (gmenu == nullptr)
      return false;

    // find the position
    pos = -1;
    auto mm = G_MENU_MODEL(m_submenu);
    for (int i=0, n=g_menu_model_get_n_items(mm); i<n; ++i)
      {
        gchar * uid = nullptr;
        if (g_menu_model_get_item_attribute(mm, i, ATTRIBUTE_X_UID, "s", &uid, nullptr))
          {
            const bool match = uid && *uid && (id==uid);
            g_free(uid);
            if (match)
              {
                pos = i;
                break;
              }
          }
      }

    return true;
  }
#endif

  bool find_current_section(const Transfer::Id& id, Section& section) const
  {
    const auto it = m_visible_transfers.find(id);
    if (it == m_visible_transfers.end())
      return false;

    section = it->second;
    return true;
  }

  void find_menuitem(const Transfer::Id& id, const Section& section, GMenu*& gmenu, int& pos) const
  {
    GMenuModel* mm = g_menu_model_get_item_link(G_MENU_MODEL(m_submenu), section, G_MENU_LINK_SECTION);
    gmenu = G_MENU(mm);

    pos = -1;
    for (int i=0, n=g_menu_model_get_n_items(mm); i<n; ++i)
      {
        gchar * uid = nullptr;

        if (g_menu_model_get_item_attribute(mm, i, ATTRIBUTE_X_UID, "s", &uid, nullptr))
          {
            const bool match = uid && *uid && (id==uid);
            g_free(uid);
            if (match)
              {
                pos = i;
                break;
              }
          }
      }
  }

  void add (const Transfer::Id& id)
  {
    update(id);
  }

  void remove(const Transfer::Id& id)
  {
    Section section = NUM_SECTIONS;
    if (!find_current_section(id, section))
      return;

    GMenu* menu = nullptr;
    int pos = -1;
    find_menuitem(id, section, menu, pos);
    if (pos < 0)
      return;

    g_message("%s removing '%s' in section #%d", G_STRLOC, id.c_str(), (int)section);
    g_menu_remove(menu, pos);
    m_visible_transfers.erase(id);
    update_bulk_menuitem(menu, section);
    update_header_soon();
  }

  void get_next_bulk_action (GMenu* menu, Section section,
                             const char*& label,
                             const char*& extra_label,
                             const char*& detailed_action) const
  {
    unsigned int n_can_pause = 0;
    unsigned int n_can_resume = 0;
    unsigned int n_can_clear = 0;
    GMenuModel* mm = G_MENU_MODEL(menu);
    for (int i=0, n=g_menu_model_get_n_items(mm); i<n; ++i)
      {
        gchar * uid = nullptr;
        if (g_menu_model_get_item_attribute(mm, i, ATTRIBUTE_X_UID, "s", &uid, nullptr))
          {
            std::shared_ptr<Transfer> t = m_model->get(uid);
            g_free(uid);
            if (!t)
              continue;

            if (t->can_pause())
              ++n_can_pause;
            if (t->can_resume())
              ++n_can_resume;
            if (t->can_clear())
              ++n_can_clear;
          }
      }

    if ((section == SUCCESSFUL) && (n_can_clear > 0))
      {
        label = _("Successful Transfers");
        extra_label = _("Clear all");
        detailed_action = "indicator.clear-all";
      }
    else if ((section == ONGOING) && (n_can_resume > 0))
      {
        extra_label = _("Resume all");
        detailed_action = "indicator.resume-all";
      }
    else if ((section == ONGOING) && (n_can_pause > 0))
      {
        extra_label = _("Pause all");
        detailed_action = "indicator.pause-all";
      }
  }

  void update_bulk_menuitem(GMenu* menu, Section section)
  {
    const char* label = nullptr;
    const char* extra_label = nullptr;
    const char* detailed_action = nullptr;
    get_next_bulk_action (menu, section, label, extra_label, detailed_action);

    // See if we've already got a bulk menuitem in this section.
    // It's always the first menuitem in the section, so look there.
    GMenuModel* mm = G_MENU_MODEL(menu);
    if (g_menu_model_get_n_items(mm) > 0)
      {
        gchar* old_extra_label = nullptr;
        if (g_menu_model_get_item_attribute(mm, 0, ATTRIBUTE_X_LABEL, "s", &old_extra_label, nullptr))
          {
            const bool equal = !g_strcmp0 (extra_label, old_extra_label);
            if (!equal) // remove the old bulk menuitem
              {
                g_message("%s removing old bulk menuitem '%s'", G_STRLOC, old_extra_label);
                g_menu_remove (menu, 0); // remove the one that we're replacing
              }
            g_free (old_extra_label);
            if (equal) // we've already got what we need... no change is necessary
              return;
          }
      }

    // if the section needs a bulk action, add it
    if (detailed_action != nullptr)
      {
        g_message("%s inserting new bulk menuitem '%s'", G_STRLOC, extra_label);
        auto item = create_bulk_action_menuitem(label, extra_label, detailed_action);
        g_menu_insert_item(menu, 0, item);
        g_object_unref(item);
      }
  }

  void update(const Transfer::Id& id)
  {
    const auto t = m_model->get(id);
    g_return_if_fail(t);

    // see if the menuitem already exists
    Section cur_section = NUM_SECTIONS;
    GMenu* cur_menu = nullptr;
    int cur_pos = -1;
    if (find_current_section(id, cur_section))
      find_menuitem(id, cur_section, cur_menu, cur_pos);

    // see where the menuitem should be
    const Section new_section = get_transfer_section(t);
    GMenu* new_menu = nullptr;
    int new_pos = -1;
    find_menuitem(id, new_section, new_menu, new_pos);

    // maybe remove from old section
    if ((cur_menu != nullptr) && (cur_menu != new_menu))
      {
        m_visible_transfers.erase(id);
        if (cur_pos >= 0)
          {
            g_message("%s removing '%s' from section #%d", G_STRLOC, id.c_str(), (int)cur_section);
            g_menu_remove (cur_menu, cur_pos);
            update_bulk_menuitem(cur_menu, cur_section);
          }
      }

    if (new_menu != nullptr)
      {
        auto item = create_transfer_menuitem(t);

        if (new_pos < 0) // hm, not in the menu yet...?
          {
            // transfers are sorted newest-to-oldest,
            // so position this one immediately after the bulk menuitem
            constexpr int insert_pos = 1;
            g_message("%s adding '%s' in section #%d", G_STRLOC, id.c_str(), (int)new_section);
            g_menu_insert_item(new_menu, insert_pos, item);
          }
        else if (!transfer_menuitem_equal(G_MENU_MODEL(new_menu), new_pos, item))
          {
            g_message("%s replacing '%s' in section #%d", G_STRLOC, id.c_str(), (int)new_section);
            g_menu_remove(new_menu, new_pos);
            g_menu_insert_item(new_menu, new_pos, item);
          }

        g_object_unref(item);
        m_visible_transfers[t->id] = new_section;
        update_bulk_menuitem(new_menu, new_section);
      }

    update_header_soon();
  }

  /***
  ****
  ***/

  // FIXME: this is a placeholder.
  // remove it when we have real icons for the menuitems
  GVariant* create_image_missing_icon()
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
  std::map<Transfer::Id,Section> m_visible_transfers;
  GMenu* m_submenu = nullptr;

  guint m_update_header_tag = 0;

  // we've got raw pointers in here, so disable copying
  Menu(const Menu&) =delete;
  Menu& operator=(const Menu&) =delete;

  static constexpr char const * ATTRIBUTE_X_UID {"x-canonical-uid"};
  static constexpr char const * ATTRIBUTE_X_TYPE {"x-canonical-type"};
  static constexpr char const * ATTRIBUTE_X_LABEL {"x-canonical-extra-label"};
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

