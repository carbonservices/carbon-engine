#include "HTTPServer.h"

// Qt
#include <QCoreApplication>
#include <QJsonObject>
#include <QJsonDocument>
#include <QtConcurrent>
#include <QTcpServer>
#include <QHostAddress>
#include <QUrlQuery>
#include <QFile>

// Templating
#include <inja/inja.hpp>
#include <nlohmann/json.hpp>
#include <unordered_set>

HttpServer::HttpServer(Database &db, LlamaEngine &llama, VectorIndex &index,
                       SystemMonitor &monitor, QObject *parent)
    : QObject(parent), _db(db), _llama(llama), _index(index), _monitor(monitor)
{
    _server.route("/api/ingest", QHttpServerRequest::Method::Post,
                  [this](const QHttpServerRequest &req)
                  { return handleIngest(req); });

    _server.route("/api/chat", QHttpServerRequest::Method::Post,
                  [this](const QHttpServerRequest &req)
                  { return handleChat(req); });

    _server.route("/api/report", QHttpServerRequest::Method::Post,
                  [this](const QHttpServerRequest &req)
                  { return handleReport(req); });

    _server.route("/api/jobs", QHttpServerRequest::Method::Get,
                  [this](const QHttpServerRequest &req)
                  { return handleJobStatus(req); });

    _server.route("/api/system", QHttpServerRequest::Method::Get,
                  [this](const QHttpServerRequest &req)
                  { return handleSystemStatus(req); });
}

bool HttpServer::listen(quint16 port)
{
    auto *tcpServer = new QTcpServer(this);
    if (!tcpServer->listen(QHostAddress::Any, port))
    {
        _log.error(QString("Failed to bind to port %1").arg(port));
        return false;
    }

    bool bound = _server.bind(tcpServer);
    if (bound)
    {
        _log.info(QString("Listening on port %1").arg(port));
    }
    return bound;
}

QFuture<QHttpServerResponse> HttpServer::handleIngest(const QHttpServerRequest &request)
{
    auto doc = QJsonDocument::fromJson(request.body());
    QString filePath = doc.object().value("file_path").toString();

    return QtConcurrent::run([this, filePath]()
    {
        if (filePath.isEmpty()) {
            _log.error("Ingest request missing file_path");
            return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);
        }

        int jobId = _db.enqueueJob(filePath);
        if (jobId < 0) {
            _log.error(QString("Failed to enqueue job for: %1").arg(filePath));
            return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);
        }
        
        _log.info(QString("Job %1 queued for: %2").arg(jobId).arg(filePath));
        return QHttpServerResponse(QJsonObject{{"status", "QUEUED"}, {"job_id", jobId}});
    });
}

QFuture<QHttpServerResponse> HttpServer::handleChat(const QHttpServerRequest &request)
{
    auto doc = QJsonDocument::fromJson(request.body());
    QString prompt = doc.object().value("prompt").toString();

    return QtConcurrent::run([this, prompt]()
    {
        if (prompt.isEmpty()) {
            _log.error("Chat request missing prompt");
            return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);
        }

        _log.info("Chat request received");
        std::string promptStd = prompt.toStdString();

        // Load embedding, search, unload, then load gen for completion.
        if (!_llama.loadEmbdModel()) {
            return QHttpServerResponse(QHttpServerResponder::StatusCode::InternalServerError);
        }
        auto queryEmbedding = _llama.generateEmbedding(promptStd);
        auto matchedIds     = _index.search(queryEmbedding, 3);
        _llama.unloadEmbdModel();

        if (!_llama.loadGenModel()) {
            return QHttpServerResponse(QHttpServerResponder::StatusCode::InternalServerError);
        }
        std::string generatedOutput;
        _llama.streamCompletion(promptStd, [&generatedOutput](const std::string& token) {
            generatedOutput += token;
            return true;
        });
        _llama.unloadGenModel();

        _log.info("Chat response generated");
        return QHttpServerResponse(QJsonObject{
            {"response", QString::fromStdString(generatedOutput)},
            {"context_matches", static_cast<int>(matchedIds.size())}
        });
    });
}

static const char* kDefaultBrsrSchemaJson = R"json({"brsr_core_kpis":[],"business_activities":[],"csr_projects":[],"external_assessment":[],"gap_reasons":[],"material_issues":[],"p1_concentration":[],"p1_fines":[],"p1_training":[],"p2_lca":[],"p2_reclaim":[],"p2_recycled_input":[],"p3_grievance_mechanisms":[],"p3_hs_training":[],"p3_parental_leave_retention":[],"p3_performance_reviews":[],"p3_retirement_benefits":[],"p3_safety_incidents":[],"p3_union_membership":[],"p3_wellbeing":[],"p4_stakeholder_engagement":[],"p5_hr_assessments":[],"p5_hr_complaints":[],"p5_hr_training":[],"p5_minimum_wages":[],"p5_remuneration":[],"p6_air_emissions":[],"p6_eia":[],"p6_energy":[],"p6_ghg_emissions":[],"p6_scope123_emissions":[],"p6_waste":[],"p6_water_discharge":[],"p6_water_withdrawal":[],"p7_anticompetitive_cases":[],"p7_policy_advocacy":[],"p7_trade_associations":[],"p8_csr_project_details":[],"p8_rehabilitation":[],"p8_sia":[],"p9_complaints":[],"p9_product_recalls":[],"penalties":[],"principle_policy_matrix":[],"products_sold":[],"review_by_committee":[],"stakeholder_grievances":[],"subsidiaries":[],"turnover_rates":[],"customer_types":"N/A","p6_pat_scheme_details":"N/A","da_emp_perm_female_pct":"N/A","p8_msme_sourcing_prev":"N/A","p2_adverse_impact_mitigation":"N/A","p3_wc_filed_prev":"N/A","p9_cyber_security_policy":"N/A","p5_hr_in_contracts":"N/A","wrk_other_female_no":"N/A","international_total":"N/A","wrk_perm_male_pct":"N/A","wrk_perm_female_pct":"N/A","director_esg_statement":"N/A","p6_sustainability_projects":"N/A","csr_turnover_amount":"N/A","p2_capex_improvements":"N/A","da_wrk_other_male_pct":"N/A","p6_value_chain_env_assessment_pct":"N/A","p3_transition_assistance_programs":"N/A","p9_service_disruption_mechanism":"N/A","emp_perm_male_no":"N/A","da_emp_perm_female_no":"N/A","esg_assessment_scope":"N/A","da_wrk_perm_female_pct":"N/A","p6_circular_economy_initiatives":"N/A","da_emp_other_male_pct":"N/A","da_emp_perm_total":"N/A","p6_climate_bcp":"N/A","p8_local_sourcing_current":"N/A","p3_corrective_actions":"N/A","da_wrk_perm_male_no":"N/A","p3_wc_pending_current":"N/A","p3_accessibility_details":"N/A","markets_national_states":"N/A","emp_perm_male_pct":"N/A","p8_local_supplier_wellbeing":"N/A","da_emp_other_female_no":"N/A","wrk_total_female_pct":"N/A","p4_consultation_process":"N/A","p4_stakeholder_identification_process":"N/A","p8_msme_sourcing_current":"N/A","p3_hs_pending_prev":"N/A","entity_name":"N/A","contact_email":"N/A","paid_up_capital":"N/A","da_wrk_female_no":"N/A","p9_consumer_feedback_mechanism":"N/A","p6_zld_status":"N/A","p6_value_chain_adverse_impact":"N/A","p5_anti_retaliation_mechanism":"N/A","p1_accounts_payable_days":"N/A","da_wrk_other_total":"N/A","national_total":"N/A","wrk_total_male_pct":"N/A","p9_info_channels":"N/A","csr_unspent_amount":"N/A","p6_business_continuity_plan":"N/A","da_wrk_perm_male_pct":"N/A","assurance_statement_summary":"N/A","p5_focal_point_details":"N/A","p2_rd_improvements":"N/A","da_emp_perm_male_pct":"N/A","sustainability_committee_details":"N/A","emp_total_female_no":"N/A","da_emp_female_pct":"N/A","p3_hs_pending_current":"N/A","p6_certifications_audits":"N/A","da_emp_male_no":"N/A","contact_telephone":"N/A","p2_rd_pct_current":"N/A","contact_person_details":"N/A","da_wrk_other_female_pct":"N/A","da_wrk_male_no":"N/A","p3_safe_workplace_measures":"N/A","da_wrk_perm_female_no":"N/A","p2_epr_status":"N/A","da_emp_perm_male_no":"N/A","p3_equal_opportunity_policy":"N/A","p5_value_chain_hr_assessment":"N/A","da_emp_other_total":"N/A","wrk_perm_total":"N/A","p3_wc_filed_current":"N/A","prior_to_previous_fy":"N/A","p3_hs_filed_prev":"N/A","p3_wc_pending_prev":"N/A","p1_conflict_of_interest_process":"N/A","wrk_total_male_no":"N/A","markets_international_countries":"N/A","p1_value_chain_awareness":"N/A","emp_total_male_no":"N/A","year_of_incorporation":"N/A","previous_renewable_energy_pct":"N/A","entity_cin":"N/A","p4_vulnerable_groups_consultation":"N/A","wrk_perm_female_no":"N/A","esg_assessment_done":"N/A","wrk_perm_male_no":"N/A","p4_consultation_outcomes":"N/A","p3_death_compensation_details":"N/A","p9_additional_product_info":"N/A","emp_other_male_pct":"N/A","p6_water_stress_area_discharge":"N/A","p6_ghg_reduction_projects":"N/A","current_renewable_energy_pct":"N/A","csr_spent_amount":"N/A","website_url":"N/A","da_emp_male_pct":"N/A","p8_local_sourcing_prev":"N/A","csr_applicable":"N/A","p3_value_chain_grievance_mechanism":"N/A","kmp_total":"N/A","p6_water_intensity_turnover_current":"N/A","da_wrk_total":"N/A","emp_total_male_pct":"N/A","da_emp_other_male_no":"N/A","emp_other_female_pct":"N/A","emp_perm_female_pct":"N/A","reporting_boundary":"N/A","p3_value_chain_assessment":"N/A","p5_grievance_mechanism":"N/A","p8_community_grievance_mechanism":"N/A","csr_net_worth":"N/A","da_wrk_perm_total":"N/A","assurance_provider_name":"N/A","emp_perm_total":"N/A","p8_negative_impact_mitigation":"N/A","governance_oversight_statement":"N/A","da_emp_total":"N/A","wrk_other_total":"N/A","emp_total_total":"N/A","p2_capex_pct_current":"N/A","p3_ohs_system_implemented":"N/A","da_wrk_other_female_no":"N/A","emp_other_total":"N/A","emp_other_male_no":"N/A","national_plants":"N/A","esg_assessment_agency":"N/A","board_female_pct":"N/A","kmp_female_no":"N/A","wrk_other_male_pct":"N/A","da_emp_female_no":"N/A","reporting_financial_year":"N/A","p9_corrective_actions":"N/A","board_female_no":"N/A","p9_data_privacy_survey_results":"N/A","international_plants":"N/A","previous_fy":"N/A","emp_other_female_no":"N/A","emp_perm_female_no":"N/A","p5_process_modification":"N/A","p3_hazard_reporting_process":"N/A","highest_authority_details":"N/A","assurance_type":"N/A","p3_value_chain_parental_leave":"N/A","p6_value_chain_corrective_actions":"N/A","p3_hazard_identification_process":"N/A","p6_environmental_compliance":"N/A","corporate_address":"N/A","p9_consumer_education_steps":"N/A","kmp_female_pct":"N/A","p2_sustainable_sourcing_process":"N/A","csr_obligation_amount":"N/A","da_wrk_male_pct":"N/A","wrk_other_female_pct":"N/A","wrk_total_female_no":"N/A","p2_rd_pct_prev":"N/A","registered_address":"N/A","p3_hs_filed_current":"N/A","p3_disability_inclusion_measures":"N/A","stock_exchanges":"N/A","p3_healthcare_access":"N/A","scope_1_2_intensity_prev":"N/A","p9_labelled_products_turnover_pct":"N/A","international_offices":"N/A","da_wrk_female_pct":"N/A","national_offices":"N/A","p2_sustainable_sourcing_pct":"N/A","da_emp_other_female_pct":"N/A","p5_corrective_actions":"N/A","export_contribution_pct":"N/A","board_total":"N/A","p6_low_carbon_transition_plan":"N/A","emp_total_female_pct":"N/A","wrk_total_total":"N/A","sustainability_decision_committee_details":"N/A","p6_water_intensity_turnover_prev":"N/A","wrk_other_male_no":"N/A","current_fy":"N/A","da_wrk_other_male_no":"N/A","p2_capex_pct_prev":"N/A"})json";

static nlohmann::json getDefaultBrsrSchema()
{
    static const nlohmann::json kSchema = nlohmann::json::parse(kDefaultBrsrSchemaJson);
    return kSchema;
}

QFuture<QHttpServerResponse> HttpServer::handleReport(const QHttpServerRequest &request)
{
    auto doc = QJsonDocument::fromJson(request.body());
    int jobId = doc.object().value("job_id").toInt(-1);

    return QtConcurrent::run([this, jobId]()
    {
        if (jobId < 0) {
            _log.error("Report request missing job_id");
            return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);
        }

        {
            QMutexLocker lock(&_reportCacheMutex);
            if (_cachedReports.contains(jobId)) {
                _log.info(QString("Serving cached report for job %1").arg(jobId));
                return QHttpServerResponse(QJsonObject{
                    {"report", _cachedReports[jobId]}
                });
            }
        }

        _log.info(QString("Targeted multi-query RAG search initiated for job %1").arg(jobId));

        // 1. Load embedding model for targeted RAG queries
        if (!_llama.loadEmbdModel()) {
            return QHttpServerResponse(QHttpServerResponder::StatusCode::InternalServerError);
        }

        std::vector<std::string> queryTexts = {
            "General Disclosures corporate name entity name financial year reporting period",
            "Scope 1 Scope 2 GHG greenhouse gas emissions turnover intensity",
            "water intensity water consumption turnover renewable energy percentage",
            "employees female percentage representation women CSR spent expenditure"
        };

        std::vector<int64_t> combinedVectorIds;
        std::unordered_set<int64_t> seenVectorIds;

        for (const auto& qText : queryTexts) {
            std::vector<float> queryEmb = _llama.generateEmbedding(qText);
            std::vector<int64_t> ids = _index.search(queryEmb, 3);
            for (int64_t vid : ids) {
                if (vid >= 0 && seenVectorIds.insert(vid).second) {
                    combinedVectorIds.push_back(vid);
                }
            }
        }

        // 2. Unload embedding model before loading generation model
        _llama.unloadEmbdModel();

        // 3. Retrieve relevant chunks from SQLite
        _monitor.setEnginePhase(EnginePhase::DbReading);
        QString assembledContext;
        for (int64_t vid : combinedVectorIds) {
            QString chunk = _db.getChunkByVectorId(vid);
            if (!chunk.isEmpty()) {
                assembledContext += chunk + "\n\n";
            }
        }
        // If vector index search returned sparse context (e.g. after server restart when in-memory index was cleared),
        // fallback to targeted keyword chunk retrieval directly from the database for the given job.
        if (assembledContext.length() < 500) {
            QVector<QString> allChunks = _db.getChunksForJob(jobId);
            if (!allChunks.isEmpty()) {
                _log.info(QString("Vector search context sparse (%1 bytes), using targeted DB chunk retrieval (%2 total chunks)")
                          .arg(assembledContext.length()).arg(allChunks.size()));

                std::vector<std::string> targetedTerms = {
                    "Corporate Identity Number",
                    "greenhouse gas emissions (Scope 1 and Scope 2",
                    "Water withdrawal, consumption and discharge",
                    "Employees and workers (including differently abled)",
                    "Details of total energy consumption (in Peta Joule)",
                    "CSR projects undertaken by your entity"
                };

                QSet<int> selectedIndices;
                for (const auto& term : targetedTerms) {
                    QString qTerm = QString::fromStdString(term);
                    for (int i = 0; i < allChunks.size(); ++i) {
                        if (allChunks[i].contains(qTerm, Qt::CaseInsensitive)) {
                            selectedIndices.insert(i);
                            break; // 1 precise chunk per category
                        }
                    }
                }

                assembledContext.clear();
                for (int idx : selectedIndices) {
                    assembledContext += allChunks[idx] + "\n\n";
                }
                _log.info(QString("Selected %1 focused context chunks (%2 characters)")
                          .arg(selectedIndices.size()).arg(assembledContext.length()));
            }
        }
        _monitor.setEnginePhase(EnginePhase::Idle);

        if (assembledContext.isEmpty()) {
            _log.error(QString("No vector context found for job %1").arg(jobId));
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
        }

        // 4. Format prompt in Qwen Instruct format
        std::string finalPrompt =
            "<|im_start|>system\n"
            "You are a strict data extraction AI for Business Responsibility and Sustainability Reporting (BRSR). Extract the following metrics from the context into a flat JSON object with exact numeric values (as plain numbers or numeric strings without unit text):\n"
            "- entity_name: Exact company name (e.g. \"Tata Steel Limited\")\n"
            "- reporting_financial_year: Reporting financial year (e.g. \"FY 2025-26\")\n"
            "- scope_1_2_intensity_current: Total Scope 1 and Scope 2 emission intensity per rupee of turnover or crude steel (e.g. \"0.0004\")\n"
            "- p6_water_intensity_turnover_current: Water intensity per rupee of turnover (e.g. \"0.000026\")\n"
            "- emp_total_female_pct: Female employees or workers percentage without % symbol (e.g. \"10.6\" or \"8.4\")\n"
            "- csr_spent_amount: CSR spent amount in INR crores without currency symbol (e.g. \"1197\" or \"142.7\")\n"
            "- current_renewable_energy_pct: Percentage or amount of energy from renewable sources (e.g. \"1.66\" or percentage if given)\n\n"
            "Rules:\n"
            "1. Output ONLY a valid JSON object starting with { and ending with }.\n"
            "2. If any metric cannot be found in the context, set its value to null.\n"
            "3. Do NOT include markdown code fences, notes, or explanations.<|im_end|>\n"
            "<|im_start|>user\n"
            "Context:\n" + assembledContext.toStdString() + "\n\nExtract JSON.<|im_end|>\n"
            "<|im_start|>assistant\n";

        _log.info(QString("Generating JSON extract for job %1...").arg(jobId));

        // 5. Load generation model and generate
        if (!_llama.loadGenModel()) {
            return QHttpServerResponse(QHttpServerResponder::StatusCode::InternalServerError);
        }

        std::string generatedJson;
        int braceCount = 0;
        bool jsonStarted = false;
        _llama.streamCompletion(finalPrompt, [&](const std::string& token) -> bool {
            generatedJson += token;
            for (char c : token) {
                if (c == '{') {
                    braceCount++;
                    jsonStarted = true;
                } else if (c == '}') {
                    braceCount--;
                }
            }
            if (jsonStarted && braceCount == 0) {
                return false; // Stop immediately once complete JSON object is closed
            }
            return true;
        });
        _llama.unloadGenModel();

        _log.info(QString("Generated BRSR extract:\n%1").arg(QString::fromStdString(generatedJson)));

        // Strip any trailing ChatML tags
        size_t endPos = generatedJson.find("<|im_end|>");
        if (endPos != std::string::npos) {
            generatedJson = generatedJson.substr(0, endPos);
        }

        // Extract JSON substring between first { and last }
        size_t firstBrace = generatedJson.find('{');
        size_t lastBrace = generatedJson.rfind('}');
        if (firstBrace != std::string::npos && lastBrace != std::string::npos && lastBrace >= firstBrace) {
            generatedJson = generatedJson.substr(firstBrace, lastBrace - firstBrace + 1);
        }

        // 6. Parse JSON from LLM output
        nlohmann::json extractedData;
        try {
            extractedData = nlohmann::json::parse(generatedJson);
        } catch (const std::exception& e) {
            _log.error(QString("JSON Parse Error: %1. Raw text: %2").arg(e.what(), QString::fromStdString(generatedJson)));
            return QHttpServerResponse(QJsonObject{
                {"report", QString("<h1>Failed to parse LLM JSON output</h1><p><b>Error:</b> %1</p><pre>%2</pre>")
                    .arg(e.what(), QString::fromStdString(generatedJson))}
            });
        }

        // Sanitize extracted data: clean numbers, strip symbols, replace null with "N/A"
        for (auto& [k, v] : extractedData.items()) {
            if (v.is_null()) {
                v = "N/A";
            } else if (v.is_string()) {
                std::string s = v.get<std::string>();
                if (k == "csr_spent_amount" || k == "emp_total_female_pct" ||
                    k == "scope_1_2_intensity_current" || k == "p6_water_intensity_turnover_current" ||
                    k == "current_renewable_energy_pct") {
                    std::string clean;
                    for (char c : s) {
                        if ((c >= '0' && c <= '9') || c == '.') clean += c;
                    }
                    if (!clean.empty()) {
                        v = clean;
                    }
                }
            }
        }

        // 7. Render HTML via Inja
        std::string renderedHtml;
        try {
            QFile file(QCoreApplication::applicationDirPath() + "/template.html");
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                _log.error("template.html not found on disk");
                return QHttpServerResponse(QJsonObject{
                    {"report", "<h1>Error: template.html not found on disk</h1>"}
                });
            }

            std::string tmpl = file.readAll().toStdString();
            inja::Environment env;

            env.add_callback("range", 2, [](inja::Arguments& args) {
                int start = args.at(0)->get<int>();
                int end = args.at(1)->get<int>();
                nlohmann::json res = nlohmann::json::array();
                for (int i = start; i < end; ++i) {
                    res.push_back(i);
                }
                return res;
            });

            // Populate base schema to ensure all 200+ template keys exist
            nlohmann::json reportData = getDefaultBrsrSchema();
            reportData.merge_patch(extractedData);

            renderedHtml = env.render(tmpl, reportData);
        } catch (const std::exception& e) {
            _log.error(QString("Template Render Error: %1").arg(e.what()));
            renderedHtml = "<h1>Template Rendering Error</h1><p>" + std::string(e.what()) + "</p>";
        }

        {
            QMutexLocker lock(&_reportCacheMutex);
            _cachedReports[jobId] = QString::fromStdString(renderedHtml);
        }

        _log.info(QString("Report generated for job %1").arg(jobId));
        return QHttpServerResponse(QJsonObject{
            {"report", QString::fromStdString(renderedHtml)}
        });
    });
}

QFuture<QHttpServerResponse> HttpServer::handleJobStatus(const QHttpServerRequest &request)
{
    bool ok = false;
    int jobId = request.query().queryItemValue("job_id").toInt(&ok);

    if (!ok || jobId < 0)
    {
        _log.error("Job status request missing job_id query parameter");
        return QtFuture::makeReadyFuture(QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest));
    }

    QString status = _db.getJobStatus(jobId);
    if (status.isEmpty())
    {
        _log.error(QString("Job status requested for unknown job %1").arg(jobId));
        return QtFuture::makeReadyFuture(QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound));
    }

    return QtFuture::makeReadyFuture(QHttpServerResponse(QJsonObject{{"status", status}}));
}

QFuture<QHttpServerResponse> HttpServer::handleSystemStatus(const QHttpServerRequest& /*request*/)
{
    double  cpuProc    = _monitor.cpuPctProcess();
    double  cpuSys     = _monitor.cpuPctSystem();
    quint64 memProc    = _monitor.memUsedMbProcess();
    quint64 memSys     = _monitor.memUsedMbSystem();
    quint64 memTotal   = _monitor.memTotalMbSystem();
    bool    gpuPres    = _monitor.gpuPresent();
    double  tps        = _monitor.tokensPerSec();
    QString phase      = SystemMonitor::phaseString(_monitor.enginePhase());
    int     queueDepth = _db.getPendingJobCount();

    QJsonObject obj;
    obj["cpu_pct_process"]        = qRound(cpuProc * 10.0) / 10.0;
    obj["cpu_pct_system"]         = qRound(cpuSys  * 10.0) / 10.0;
    obj["memory_used_mb_process"] = static_cast<qint64>(memProc);
    obj["memory_used_mb_system"]  = static_cast<qint64>(memSys);
    obj["memory_total_mb_system"] = static_cast<qint64>(memTotal);
    obj["gpu_present"]            = gpuPres;
    obj["gpu_pct"]                = gpuPres ? QJsonValue(0.0) : QJsonValue(QJsonValue::Null);
    obj["job_queue_depth"]        = queueDepth;
    obj["engine_status"]          = phase;
    obj["db_status"]              = "Idle";
    obj["tokens_per_sec"]         = qRound(tps * 10.0) / 10.0;

    return QtFuture::makeReadyFuture(QHttpServerResponse(obj));
}