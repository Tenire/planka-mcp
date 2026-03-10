#include "registry.h"
#include <regex>
#include <logger.h>
#include "../resources/registry.h"

namespace mcp {

ToolRegistry::ToolRegistry(ResourceRegistry& resource_registry) 
    : resource_registry_(resource_registry) {
    init_definitions();
}

std::vector<Tool> ToolRegistry::list_tools() {
    std::vector<Tool> tools;
    
    // 1. planka_create
    wfrest::Json create_schema = wfrest::Json::Object();
    create_schema.push_back("type", "object");
    wfrest::Json c_props = wfrest::Json::Object();
    wfrest::Json req = wfrest::Json::Array();
    
    wfrest::Json et = wfrest::Json::Object();
    et.push_back("type", "string");
    et.push_back("description", "Type of entity to create (e.g., project, board, list, card, task_list, task, comment, user, webhook, label, etc.)");
    c_props.push_back("entity_type", et);
    req.push_back("entity_type");

    wfrest::Json pid = wfrest::Json::Object();
    pid.push_back("type", "string");
    pid.push_back("description", "ID of the parent entity, if applicable (e.g., projectId for a board, boardId for a list, listId for a card). Read resources for exact parent_id to use.");
    c_props.push_back("parent_id", pid);

    wfrest::Json d = wfrest::Json::Object();
    d.push_back("type", "object");
    d.push_back("description", "Fields for creation (e.g., name, description).");
    c_props.push_back("data", d);
    req.push_back("data");

    create_schema.push_back("properties", c_props);
    create_schema.push_back("required", req);
    tools.push_back({"planka_create", "Create a new Planka entity.", create_schema});

    // 2. planka_update
    wfrest::Json update_schema = wfrest::Json::Object();
    update_schema.push_back("type", "object");
    wfrest::Json u_props = wfrest::Json::Object();
    wfrest::Json u_req = wfrest::Json::Array();
    
    u_props.push_back("entity_type", et);
    u_req.push_back("entity_type");

    wfrest::Json uid = wfrest::Json::Object();
    uid.push_back("type", "string");
    uid.push_back("description", "ID of the entity to update or delete.");
    u_props.push_back("id", uid);
    u_req.push_back("id");

    u_props.push_back("data", d);
    u_req.push_back("data");

    update_schema.push_back("properties", u_props);
    update_schema.push_back("required", u_req);
    tools.push_back({"planka_update", "Update an existing Planka entity.", update_schema});

    // 3. planka_delete
    wfrest::Json delete_schema = wfrest::Json::Object();
    delete_schema.push_back("type", "object");
    wfrest::Json del_props = wfrest::Json::Object();
    wfrest::Json del_req = wfrest::Json::Array();

    del_props.push_back("entity_type", et);
    del_req.push_back("entity_type");

    del_props.push_back("id", uid);
    del_req.push_back("id");

    delete_schema.push_back("properties", del_props);
    delete_schema.push_back("required", del_req);
    tools.push_back({"planka_delete", "Delete an existing Planka entity.", delete_schema});

    // 4. planka_action
    wfrest::Json action_schema = wfrest::Json::Object();
    action_schema.push_back("type", "object");
    wfrest::Json a_props = wfrest::Json::Object();
    wfrest::Json a_req = wfrest::Json::Array();

    wfrest::Json action_name = wfrest::Json::Object();
    action_name.push_back("type", "string");
    action_name.push_back("description", "Action to perform (e.g., duplicate_card, move_cards_in_list, add_card_member, add_card_label).");
    a_props.push_back("action", action_name);
    a_req.push_back("action");

    wfrest::Json a_data = wfrest::Json::Object();
    a_data.push_back("type", "object");
    a_data.push_back("description", "Arguments required for the action.");
    a_props.push_back("data", a_data);

    action_schema.push_back("properties", a_props);
    action_schema.push_back("required", a_req);
    tools.push_back({"planka_action", "Perform specialized actions/relationships (move, sort, membership, custom fields).", action_schema});

    // 5. planka_explore
    wfrest::Json explore_schema = wfrest::Json::Object();
    explore_schema.push_back("type", "object");
    wfrest::Json e_props = wfrest::Json::Object();
    
    wfrest::Json uri_field = wfrest::Json::Object();
    uri_field.push_back("type", "string");
    uri_field.push_back("description", "Resource URI to explore (e.g., planka://projects).");
    e_props.push_back("uri", uri_field);

    wfrest::Json templates_field = wfrest::Json::Object();
    templates_field.push_back("type", "boolean");
    templates_field.push_back("description", "Set to true to list available URI templates instead of roots.");
    e_props.push_back("templates", templates_field);

    explore_schema.push_back("properties", e_props);
    tools.push_back({"planka_explore", "Explore resources or list discovery paths. Call with templates:true to see URI templates.", explore_schema});

    return tools;
}

coke::Task<wfrest::Json> ToolRegistry::call_tool(const std::string& name, const wfrest::Json& arguments, PlankaClient& client) {
    auto make_err = [](const std::string& msg) {
        wfrest::Json response_content = wfrest::Json::Array();
        wfrest::Json item = wfrest::Json::Object();
        item.push_back("type", "text");
        item.push_back("text", msg);
        response_content.push_back(item);
        return response_content;
    };

    if (name == "planka_explore") {
        std::string uri = "";
        bool show_templates = false;

        if (arguments.has("uri") && arguments["uri"].is_string()) {
            uri = arguments["uri"].get<std::string>();
        }
        if (arguments.has("templates") && arguments["templates"].is_boolean()) {
            show_templates = arguments["templates"].get<bool>();
        }

        if (uri.empty()) {
            wfrest::Json discovery = wfrest::Json::Object();
            if (show_templates) {
                wfrest::Json templates = wfrest::Json::Array();
                for (const auto& t : resource_registry_.list_templates()) {
                    wfrest::Json tj = wfrest::Json::Object();
                    tj.push_back("uriTemplate", t.uriTemplate);
                    tj.push_back("name", t.name);
                    tj.push_back("description", t.description);
                    templates.push_back(tj);
                }
                discovery.push_back("availableTemplates", templates);
                discovery.push_back("_hint", "Replace {id} with actual IDs. For creation, use planka_create with entity_type (e.g., project, board, list, card, board_membership). Projects require 'type' (private or shared).");
            } else {
                wfrest::Json roots = wfrest::Json::Array();
                for (const auto& r : resource_registry_.list_resources()) {
                    wfrest::Json rj = wfrest::Json::Object();
                    rj.push_back("uri", r.uri);
                    rj.push_back("name", r.name);
                    rj.push_back("description", r.description);
                    roots.push_back(rj);
                }
                discovery.push_back("availableRoots", roots);
                discovery.push_back("_hint", "Call with templates:true to see all URI templates, or provide a specific URI to explore data.");
            }

            wfrest::Json content = wfrest::Json::Array();
            wfrest::Json item = wfrest::Json::Object();
            item.push_back("type", "text");
            item.push_back("text", discovery.dump(4));
            content.push_back(item);
            co_return content;
        }
        
        wfrest::Json resource_data = co_await resource_registry_.read_resource(uri, client);
        if (resource_data.is_array() && !resource_data.empty()) {
            wfrest::Json tool_content = wfrest::Json::Array();
            for (const auto& item : resource_data) {
                wfrest::Json text_item = wfrest::Json::Object();
                text_item.push_back("type", "text");
                // Resource response from read_resource has "text" field
                if (item.has("text")) {
                    text_item.push_back("text", item["text"]);
                } else {
                    text_item.push_back("text", item.dump());
                }
                tool_content.push_back(text_item);
            }
            co_return tool_content;
        } else {
            co_return make_err("Error: Resource not found or unsupported URI for explore: " + uri);
        }
    }

    if (name == "planka_create" || name == "planka_update" || name == "planka_delete") {
        if (!arguments.has("entity_type") || !arguments["entity_type"].is_string()) {
            co_return make_err("Error: Missing entity_type in arguments");
        }
        std::string etype = arguments["entity_type"].get<std::string>();
        std::string internal_name = "";
        
        if (name == "planka_create") internal_name = "create_" + etype;
        else if (name == "planka_update") internal_name = "update_" + etype;
        else if (name == "planka_delete") internal_name = "delete_" + etype;
        
        wfrest::Json internal_args = arguments.has("data") && arguments["data"].is_object() ? arguments["data"] : wfrest::Json::Object();
        
        if (arguments.has("id")) {
            internal_args.push_back("id", arguments["id"]);
        }
        if (arguments.has("parent_id") && arguments["parent_id"].is_string()) {
             std::string parent_id = arguments["parent_id"].get<std::string>();
             if (etype == "board") internal_args.push_back("projectId", parent_id);
             else if (etype == "list") internal_args.push_back("boardId", parent_id);
             else if (etype == "card") internal_args.push_back("listId", parent_id);
             else if (etype == "task_list") internal_args.push_back("cardId", parent_id);
             else if (etype == "task") internal_args.push_back("taskListId", parent_id);
             else if (etype == "comment") internal_args.push_back("cardId", parent_id);
             else if (etype == "label" || etype == "board_custom_field_group") internal_args.push_back("boardId", parent_id);
             else if (etype == "card_custom_field_group") internal_args.push_back("cardId", parent_id);
             else if (etype == "custom_field") internal_args.push_back("customFieldGroupId", parent_id);
             else if (etype == "base_custom_field") internal_args.push_back("baseCustomFieldGroupId", parent_id);
             else if (etype == "attachment") internal_args.push_back("cardId", parent_id);
             else if (etype == "board_notification_service") internal_args.push_back("boardId", parent_id);
             else if (etype == "user_notification_service") internal_args.push_back("userId", parent_id);
             else if (etype == "project_manager") internal_args.push_back("projectId", parent_id);
             else if (etype == "board_membership") internal_args.push_back("boardId", parent_id);
             else if (etype == "card_membership") internal_args.push_back("cardId", parent_id);
             else if (etype == "card_label") internal_args.push_back("cardId", parent_id);
         }

        for (const auto& def : definitions_) {
            if (def.name == internal_name) {
                co_return co_await execute_generic(def, internal_args, client);
            }
        }
        co_return make_err("Error: Unknown entity type or operation not supported: " + internal_name);
    } 
    else if (name == "planka_action") {
        if (!arguments.has("action") || !arguments["action"].is_string()) {
            co_return make_err("Error: Missing action in arguments");
        }
        std::string internal_name = arguments["action"].get<std::string>();
        // Name normalization for backward compatibility and internal consistency
        if (internal_name == "setup_card_member" || internal_name == "add_card_member") {
            internal_name = "create_card_membership";
        } else if (internal_name == "add_card_label") {
            internal_name = "create_card_label";
        }

        wfrest::Json internal_args = arguments.has("data") && arguments["data"].is_object() ? arguments["data"] : wfrest::Json::Object();
        
        for (const auto& def : definitions_) {
            if (def.name == internal_name) {
                co_return co_await execute_generic(def, internal_args, client);
            }
        }
        co_return make_err("Error: Unknown action: " + internal_name);
    }

    co_return make_err("Error: Tool not found: " + name);
}

coke::Task<wfrest::Json> ToolRegistry::execute_generic(const ToolDef& def, const wfrest::Json& args, PlankaClient& client) {
    auto make_raw_err = [](const std::string& msg) {
        wfrest::Json response_content = wfrest::Json::Array();
        wfrest::Json item = wfrest::Json::Object();
        item.push_back("type", "text");
        item.push_back("text", msg);
        response_content.push_back(item);
        return response_content;
    };

    std::string path = def.path;
    
    // Replace URL parameters like {id}
    std::regex param_regex(R"(\{([^}]+)\})");
    std::string current_path = path;
    std::smatch match;
    bool all_params_replaced = true;
    while (std::regex_search(current_path, match, param_regex)) {
        std::string param_name = match[1];
        if (args.has(param_name)) {
            std::string val = args[param_name].get<std::string>();
            current_path.replace(match.position(), match.length(), val);
        } else {
            all_params_replaced = false;
            break;
        }
    }

    if (!all_params_replaced) {
        co_return make_raw_err("Error: Missing required URL parameter in: " + path);
    }
    path = current_path;
    LOG_INFO() << "[ToolRegistry] " << def.name << " | Args: " << args.dump();
    LOG_INFO() << "[ToolRegistry] Calling API: " << def.method << " " << path;

    // Construct request body from remaining args
    wfrest::Json body = wfrest::Json::Object();
    for (const auto& field : def.fields) {
        // Only add to body if field is NOT a path parameter (like {id})
        if (args.has(field.name) && def.path.find("{" + field.name + "}") == std::string::npos) {
            body.push_back(field.name, args[field.name]);
        }
    }

    // Default position/type if needed for creation (Researched: Planka uses Numeric GAP=16384)
    if (def.method == "POST") {
        bool is_card_creation = (path.find("/api/lists/") != std::string::npos && path.find("/cards") != std::string::npos && path.find("/attachments") == std::string::npos);
        if (is_card_creation || 
            path.find("/api/boards/") != std::string::npos && path.find("/lists") != std::string::npos ||
            path.find("/api/projects/") != std::string::npos && path.find("/boards") != std::string::npos) {
            
            if (!body.has("position"))
                body.push_back("position", 16384); // Optimized default GAP
        }

        if (is_card_creation) {
            // Cards in Planka v2 default to 'project' type if not specified
            if (!body.has("type"))
                body.push_back("type", "project");
        }
        // ONLY lists require 'type' as 'active' or 'closed' during POST /api/boards/{id}/lists
        if (path.find("/lists") != std::string::npos && path.find("/cards") == std::string::npos) {
            if (!body.has("type"))
                body.push_back("type", "active");
        }
    }
    LOG_INFO() << "[ToolRegistry] Body: " << body.dump();

    wfrest::Json api_res;
    if (def.method == "POST") {
        api_res = co_await client.post(path, body);
    } else if (def.method == "PATCH") {
        api_res = co_await client.patch(path, body);
    } else if (def.method == "DELETE") {
        api_res = co_await client.del(path);
    } else if (def.method == "GET") {
        api_res = co_await client.get(path);
    }

    wfrest::Json response_content = wfrest::Json::Array();
    wfrest::Json item = wfrest::Json::Object();
    item.push_back("type", "text");
    
    if (!api_res.is_valid() || api_res.is_null()) {
        item.push_back("text", "Error: API returned empty or invalid response for " + def.name);
        response_content.push_back(item);
        co_return response_content;
    }

    if (api_res.has("__is_error__") && api_res["__is_error__"].get<bool>()) {
        std::string err_msg = "Error executing " + def.name + ": ";
        std::string planka_code = api_res.has("code") ? api_res["code"].get<std::string>() : "";
        std::string planka_msg = api_res.has("message") ? api_res["message"].get<std::string>() : "";

        if (planka_code == "E_NOT_FOUND" || (planka_code.empty() && planka_msg.find("not found") != std::string::npos)) {
            err_msg += "Resource or User not found. ";
            if (def.name == "create_board_membership") {
                err_msg += "Hint: Ensure the user exists in the system via planka_explore(uri:'planka://users').";
            } else if (def.name == "create_card_membership" || def.name == "create_card_label") {
                err_msg += "Hint: For card assignment, the user or label MUST already be a member of the parent BOARD. Use planka_create(entity_type:'board_membership') first.";
            } else if (def.name == "create_project_manager") {
                err_msg += "Hint: Ensure the user exists in the system.";
            } else {
                err_msg += "Check IDs and ensure the parent resource exists.";
            }
        } else if (planka_code == "E_UNPROCESSABLE_ENTITY") {
            err_msg += "Unprocessable entity (e.g., duplicate membership or missing requirements). " + planka_msg;
        } else {
            err_msg += (!planka_code.empty() ? "[" + planka_code + "] " : "") + (!planka_msg.empty() ? planka_msg : "Unknown Planka API error");
        }
        
        item.push_back("text", err_msg);
        response_content.push_back(item);
        co_return response_content;
    }

    wfrest::Json target = api_res.has("item") ? api_res["item"] : api_res;
    
    if (target.is_valid() && (target.has("id") || api_res.is_array() || api_res.is_null() || api_res.has("success")))
    {
        std::string msg = "Successfully executed " + def.name + ".";
        if (target.has("id"))
            msg += " ID: `" + target["id"].get<std::string>() + "`";
        item.push_back("text", msg);
    } else {
        item.push_back("text", "Finished executing " + def.name + " with response: " + api_res.dump());
    }
    
    response_content.push_back(item);
    co_return response_content;
}

wfrest::Json ToolRegistry::build_schema(const ToolDef& def) {
    wfrest::Json schema = wfrest::Json::Object();
    schema.push_back("type", "object");
    wfrest::Json properties = wfrest::Json::Object();
    wfrest::Json required = wfrest::Json::Array();

    for (const auto& field : def.fields) {
        wfrest::Json prop = wfrest::Json::Object();
        
        if (field.type == "checkbox") {
            prop.push_back("type", "boolean");
        } else if (field.type == "dropdown") {
            prop.push_back("type", "string");
            if (!field.options.empty()) {
                wfrest::Json enums = wfrest::Json::Array();
                for (const auto& opt : field.options) enums.push_back(opt);
                prop.push_back("enum", enums);
            }
        } else if (field.type == "date") {
            prop.push_back("type", "string");
            prop.push_back("description", field.description + " (ISO 8601 date string)");
        } else {
            prop.push_back("type", field.type);
        }
        
        prop.push_back("description", field.description);
        properties.push_back(field.name, prop);
        if (field.required) {
            required.push_back(field.name);
        }
    }

    schema.push_back("properties", properties);
    if (required.size() > 0) {
        schema.push_back("required", required);
    }
    return schema;
}

void ToolRegistry::init_definitions() {
    // Project & Board
    definitions_.push_back({"create_project", "Create a project", "POST", "/api/projects", {
        {"name", "Project name", "string", true},
        {"type", "Project type", "dropdown", true, {"private", "shared"}}
    }});

    definitions_.push_back({"update_project", "Update a project", "PATCH", "/api/projects/{id}", {
        {"id", "Project ID", "string", true},
        {"name", "New name", "string", false},
        {"description", "New description", "string", false},
        {"isHidden", "Hide project", "checkbox", false}
    }});

    definitions_.push_back({"delete_project", "Delete a project", "DELETE", "/api/projects/{id}", {
        {"id", "Project ID", "string", true}
    }});

    definitions_.push_back({"create_project_manager", "Add project manager", "POST", "/api/projects/{projectId}/project-managers", {
        {"projectId", "Project ID", "string", true},
        {"userId", "User ID", "string", true}
    }});

    definitions_.push_back({"delete_project_manager", "Remove project manager", "DELETE", "/api/project-managers/{id}", {
        {"id", "Manager ID", "string", true}
    }});

    definitions_.push_back({"create_board", "Create a board", "POST", "/api/projects/{projectId}/boards", {
        {"projectId", "Project ID", "string", true},
        {"name", "Board name", "string", true},
        {"position", "Position", "number", false}
    }});

    definitions_.push_back({"update_board", "Update a board", "PATCH", "/api/boards/{id}", {
        {"id", "Board ID", "string", true},
        {"name", "New name", "string", false},
        {"position", "New position", "number", false},
        {"isSubscribed", "Subscribe to board", "checkbox", false}
    }});

    definitions_.push_back({"delete_board", "Delete a board", "DELETE", "/api/boards/{id}", {
        {"id", "Board ID", "string", true}
    }});

    definitions_.push_back({"create_board_membership", "Add member to board", "POST", "/api/boards/{boardId}/board-memberships", {
        {"boardId", "Board ID", "string", true},
        {"userId", "User ID", "string", true},
        {"role", "Role", "dropdown", false, {"editor", "viewer"}}
    }});

    definitions_.push_back({"update_board_membership", "Update board membership", "PATCH", "/api/board-memberships/{id}", {
        {"id", "Membership ID", "string", true},
        {"role", "Role", "dropdown", true, {"editor", "viewer"}}
    }});

    definitions_.push_back({"delete_board_membership", "Remove board member", "DELETE", "/api/board-memberships/{id}", {
        {"id", "Membership ID", "string", true}
    }});

    // List & Card
    definitions_.push_back({"create_list", "Create a list", "POST", "/api/boards/{boardId}/lists", {
        {"boardId", "Board ID", "string", true},
        {"name", "List name", "string", true},
        {"position", "Position", "number", false},
        {"type", "Type", "dropdown", false, {"active", "closed"}}
    }});

    definitions_.push_back({"update_list", "Update a list", "PATCH", "/api/lists/{id}", {
        {"id", "List ID", "string", true},
        {"name", "New name", "string", false},
        {"position", "New position", "number", false},
        {"type", "Status (active, closed, archive)", "dropdown", false, {"active", "closed", "archive"}}
    }});

    definitions_.push_back({"delete_list", "Delete a list", "DELETE", "/api/lists/{id}", {
        {"id", "List ID", "string", true}
    }});

    definitions_.push_back({"clear_list", "Clear all cards from list", "DELETE", "/api/lists/{id}/cards", {
        {"id", "List ID", "string", true}
    }});

    definitions_.push_back({"sort_list", "Sort cards in list", "POST", "/api/lists/{id}/cards/sort", {
        {"id", "List ID", "string", true}
    }});

    definitions_.push_back({"move_cards_in_list", "Move all cards to another list", "POST", "/api/lists/{id}/cards/move", {
        {"id", "Source list ID", "string", true},
        {"listId", "Target list ID", "string", true}
    }});

    definitions_.push_back({"create_card", "Create a card", "POST", "/api/lists/{listId}/cards", {
        {"listId", "List ID", "string", true},
        {"name", "Card name", "string", true},
        {"description", "Optional description", "string", false},
        {"position", "Position", "number", false},
        {"type", "Card type", "dropdown", false, {"project", "story"}}
    }});

    definitions_.push_back({"update_card", "Update a card", "PATCH", "/api/cards/{id}", {
        {"id", "Card ID", "string", true},
        {"name", "New name", "string", false},
        {"description", "New description (supports Markdown)", "string", false},
        {"position", "New position", "number", false},
        {"type", "New type (project, story)", "dropdown", false, {"project", "story"}},
        {"coverAttachmentId", "Set cover image (ID)", "string", false},
        {"dueDate", "Due date (ISO string, e.g., 2026-12-31T23:59:59Z)", "string", false},
        {"isDueCompleted", "Due completed", "checkbox", false},
        {"stopwatch", "Timer state object: {startedAt: ISO/null, total: number}", "object", false},
        {"isSubscribed", "Subscribe to card", "checkbox", false}
    }});

    definitions_.push_back({"delete_card", "Delete a card", "DELETE", "/api/cards/{id}", {
        {"id", "Card ID", "string", true}
    }});

    definitions_.push_back({"move_card", "Move a card", "PATCH", "/api/cards/{id}", {
        {"id", "Card ID", "string", true},
        {"boardId", "New board ID", "string", false},
        {"listId", "New list ID", "string", true},
        {"position", "New position", "number", false}
    }});

    definitions_.push_back({"duplicate_card", "Duplicate a card", "POST", "/api/cards/{id}/duplicate", {
        {"id", "Card ID", "string", true},
        {"boardId", "Target Board ID", "string", false},
        {"listId", "Target List ID", "string", false},
        {"name", "New name", "string", false},
        {"position", "New position", "number", false}
    }});

    definitions_.push_back({"create_attachment", "Add attachment to card", "POST", "/api/cards/{cardId}/attachments", {
        {"cardId", "Card ID", "string", true},
        {"name", "Attachment name (for link)", "string", true},
        {"type", "Type (link, file)", "string", true},
        {"url", "URL (for link)", "string", false}
    }});

    // Membership & Labels & Tasks & Comments
    definitions_.push_back({"create_card_membership", "Add user as card member", "POST", "/api/cards/{cardId}/card-memberships", {
        {"cardId", "Card ID", "string", true},
        {"userId", "User ID", "string", true}
    }});

    definitions_.push_back({"delete_card_membership", "Remove card member", "DELETE", "/api/cards/{cardId}/card-memberships/userId:{userId}", {
        {"cardId", "Card ID", "string", true},
        {"userId", "User ID", "string", true}
    }});

    definitions_.push_back({"create_label", "Create a label", "POST", "/api/boards/{boardId}/labels", {
        {"boardId", "Board ID", "string", true},
        {"name", "Label name", "string", true},
        {"color", "Color", "dropdown", false, {"berry-red", "orange-peel", "egg-yellow", "fresh-salad", "midnight-blue", "lilac-eyes", "apricot-red"}},
        {"position", "Position", "number", false}
    }});

    definitions_.push_back({"update_label", "Update label details", "PATCH", "/api/labels/{id}", {
        {"id", "Label ID", "string", true},
        {"name", "New name", "string", false},
        {"color", "New color", "dropdown", false, {"berry-red", "orange-peel", "egg-yellow", "fresh-salad", "midnight-blue", "lilac-eyes", "apricot-red"}}
    }});

    definitions_.push_back({"delete_label", "Delete a label", "DELETE", "/api/labels/{id}", {
        {"id", "Label ID", "string", true}
    }});

    definitions_.push_back({"create_card_label", "Add label to card", "POST", "/api/cards/{cardId}/card-labels", {
        {"cardId", "Card ID", "string", true},
        {"labelId", "Label ID", "string", true}
    }});
    definitions_.push_back({"delete_card_label", "Remove label from card", "DELETE", "/api/cards/{cardId}/card-labels/labelId:{labelId}", {
        {"cardId", "Card ID", "string", true},
        {"labelId", "Label ID", "string", true}
    }});

    definitions_.push_back({"create_task_list", "Create task list", "POST", "/api/cards/{cardId}/task-lists", {
        {"cardId", "Card ID", "string", true},
        {"name", "Name", "string", true},
        {"position", "Position", "number", false}
    }});

    definitions_.push_back({"update_task_list", "Update task list", "PATCH", "/api/task-lists/{id}", {
        {"id", "Task list ID", "string", true},
        {"name", "New name", "string", false},
        {"position", "New position", "number", false}
    }});

    definitions_.push_back({"delete_task_list", "Delete a task list", "DELETE", "/api/task-lists/{id}", {
        {"id", "Task list ID", "string", true}
    }});

    definitions_.push_back({"create_task", "Create task", "POST", "/api/task-lists/{taskListId}/tasks", {
        {"taskListId", "Task List ID", "string", true},
        {"name", "Name", "string", true},
        {"position", "Position", "number", false},
        {"assigneeUserId", "Assignee User ID", "string", false}
    }});

    definitions_.push_back({"update_task", "Update task", "PATCH", "/api/tasks/{id}", {
        {"id", "Task ID", "string", true},
        {"name", "Name", "string", false},
        {"isCompleted", "Completed", "checkbox", false},
        {"position", "New position", "number", false},
        {"assigneeUserId", "Assignee User ID", "string", false}
    }});

    definitions_.push_back({"delete_task", "Delete a task", "DELETE", "/api/tasks/{id}", {
        {"id", "Task ID", "string", true}
    }});

    definitions_.push_back({"create_comment", "Add comment to card", "POST", "/api/cards/{cardId}/comments", {
        {"cardId", "Card ID", "string", true},
        {"text", "Comment text", "string", true}
    }});

    definitions_.push_back({"update_comment", "Update comment", "PATCH", "/api/comments/{id}", {
        {"id", "Comment ID", "string", true},
        {"text", "New text", "string", true}
    }});

    definitions_.push_back({"delete_comment", "Delete a comment", "DELETE", "/api/comments/{id}", {
        {"id", "Comment ID", "string", true}
    }});

    // Users & Notifications & Webhooks
    definitions_.push_back({"create_user", "Create a new user", "POST", "/api/users", {
        {"email", "Email", "string", true},
        {"password", "Password", "string", false},
        {"name", "Name", "string", false}
    }});

    definitions_.push_back({"update_user", "Update a user", "PATCH", "/api/users/{id}", {
        {"id", "User ID", "string", true},
        {"email", "New email", "string", false},
        {"name", "New name", "string", false}
    }});

    definitions_.push_back({"update_user_password", "Update user password", "PATCH", "/api/users/{id}/password", {
        {"id", "User ID", "string", true},
        {"password", "New Password", "string", true}
    }});

    definitions_.push_back({"delete_user", "Delete a user", "DELETE", "/api/users/{id}", {
        {"id", "User ID", "string", true}
    }});

    definitions_.push_back({"update_notification", "Update a notification", "PATCH", "/api/notifications/{id}", {
        {"id", "Notification ID", "string", true},
        {"isRead", "Mark as read", "checkbox", true}
    }});

    definitions_.push_back({"read_card_notifications", "Mark specific card notifications read", "POST", "/api/cards/{id}/notifications/read", {
        {"id", "Card ID", "string", true}
    }});

    definitions_.push_back({"mark_all_notifications_read", "Mark all notifications read", "POST", "/api/notifications/read", {}});

    definitions_.push_back({"create_webhook", "Create a webhook", "POST", "/api/webhooks", {
        {"name", "Name", "string", true},
        {"url", "URL", "string", true}
    }});

    definitions_.push_back({"update_webhook", "Update a webhook", "PATCH", "/api/webhooks/{id}", {
        {"id", "Webhook ID", "string", true},
        {"name", "New name", "string", false},
        {"url", "New URL", "string", false}
    }});

    definitions_.push_back({"delete_webhook", "Delete a webhook", "DELETE", "/api/webhooks/{id}", {
        {"id", "Webhook ID", "string", true}
    }});
 
    // Notification Services (Non-Admin Webhooks)
    definitions_.push_back({"create_board_notification_service", "Create board notification service", "POST", "/api/boards/{boardId}/notification-services", {
        {"boardId", "Board ID", "string", true},
        {"url", "URL", "string", true},
        {"format", "Format (text, markdown, html)", "dropdown", true, {"text", "markdown", "html"}}
    }});
 
    definitions_.push_back({"create_user_notification_service", "Create user notification service", "POST", "/api/users/{userId}/notification-services", {
        {"userId", "User ID", "string", true},
        {"url", "URL", "string", true},
        {"format", "Format (text, markdown, html)", "dropdown", true, {"text", "markdown", "html"}}
    }});
 
    definitions_.push_back({"update_notification_service", "Update notification service", "PATCH", "/api/notification-services/{id}", {
        {"id", "Service ID", "string", true},
        {"url", "URL", "string", false},
        {"format", "Format (text, markdown, html)", "dropdown", false, {"text", "markdown", "html"}}
    }});
 
    definitions_.push_back({"test_notification_service", "Test notification service", "POST", "/api/notification-services/{id}/test", {
        {"id", "Service ID", "string", true}
    }});
 
    definitions_.push_back({"delete_notification_service", "Delete notification service", "DELETE", "/api/notification-services/{id}", {
        {"id", "Service ID", "string", true}
    }});

    // Custom Fields
    definitions_.push_back({"create_board_custom_field_group", "Create board CF group", "POST", "/api/boards/{boardId}/custom-field-groups", {
        {"boardId", "Board ID", "string", true},
        {"name", "Name", "string", true}
    }});

    definitions_.push_back({"create_card_custom_field_group", "Create card CF group", "POST", "/api/cards/{cardId}/custom-field-groups", {
        {"cardId", "Card ID", "string", true},
        {"name", "Name", "string", true}
    }});

    definitions_.push_back({"update_custom_field_group", "Update CF group", "PATCH", "/api/custom-field-groups/{id}", {
        {"id", "Group ID", "string", true},
        {"name", "New name", "string", true}
    }});

    definitions_.push_back({"delete_custom_field_group", "Delete CF group", "DELETE", "/api/custom-field-groups/{id}", {
        {"id", "Group ID", "string", true}
    }});

    definitions_.push_back({"create_custom_field", "Create CF in group", "POST", "/api/custom-field-groups/{customFieldGroupId}/custom-fields", {
        {"customFieldGroupId", "Group ID", "string", true},
        {"name", "Field Name", "string", true},
        {"type", "Field Type", "dropdown", true, {"text", "number", "dropdown", "checkbox", "date"}},
        {"position", "Position", "number", false}
    }});

    definitions_.push_back({"create_base_custom_field", "Create base CF", "POST", "/api/base-custom-field-groups/{baseCustomFieldGroupId}/base-custom-fields", {
        {"baseCustomFieldGroupId", "Base CF Group ID", "string", true},
        {"name", "Field Name", "string", true},
        {"type", "Field Type", "dropdown", true, {"text", "number", "dropdown", "checkbox", "date"}}
    }});

    definitions_.push_back({"update_custom_field", "Update CF", "PATCH", "/api/custom-fields/{id}", {
        {"id", "Field ID", "string", true},
        {"name", "New name", "string", false},
        {"position", "New position", "number", false}
    }});

    definitions_.push_back({"delete_custom_field", "Delete CF", "DELETE", "/api/custom-fields/{id}", {
        {"id", "Field ID", "string", true}
    }});

    definitions_.push_back({"set_card_custom_field", "Set a custom field value on a card", "PATCH", "/api/cards/{cardId}/custom-field-values/customFieldGroupId:{customFieldGroupId}:customFieldId:{customFieldId}", {
        {"cardId", "Card ID", "string", true},
        {"customFieldGroupId", "Group ID", "string", true},
        {"customFieldId", "Field ID", "string", true},
        {"content", "Value/Content", "string", true}
    }});

    definitions_.push_back({"delete_card_custom_field_value", "Delete a custom field value from a card", "DELETE", "/api/cards/{cardId}/custom-field-values/customFieldGroupId:{customFieldGroupId}:customFieldId:{customFieldId}", {
        {"cardId", "Card ID", "string", true},
        {"customFieldGroupId", "Group ID", "string", true},
        {"customFieldId", "Field ID", "string", true}
    }});

    definitions_.push_back({"delete_attachment", "Delete an attachment", "DELETE", "/api/attachments/{id}", {
        {"id", "Attachment ID", "string", true}
    }});
}

} // namespace mcp
